#include "pch/CorePCH.hpp"

#include "graphics/Renderer.hpp"

#include "core/Application.hpp"
#include "core/Clock.hpp"
#include "core/Random.hpp"
#include "core/Scene.hpp"
#include "core/ShadowCascadeCalculator.hpp"

#include "logging/Logger.hpp"

#include "graphics/DescriptorResource.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

#include "graphics/Framebuffer.hpp"

#include "graphics/RendererExtensions.hpp"
#include "graphics/TextureGenerator.hpp"

#include "graphics/render_passes/Bloom.hpp"
#include "graphics/render_passes/ChromaticAberration.hpp"
#include "graphics/render_passes/Composition.hpp"
#include "graphics/render_passes/Deferred.hpp"
#include "graphics/render_passes/LightCulling.hpp"
#include "graphics/render_passes/Lights.hpp"
#include "graphics/render_passes/MainGeometry.hpp"
#include "graphics/render_passes/Predepth.hpp"
#include "graphics/render_passes/Shadow.hpp"

#include <cstddef>
#include <glm/gtc/quaternion.hpp>
#include <ranges>
#include <span>

namespace Engine::Graphics {

static constexpr auto update_lights = []<class Light>(Light& light_ubo,
                                                      auto& env_lights) {
  auto i = 0ULL;
  auto& [ubo_count, ubo_lights] = light_ubo.get_data();
  ubo_count = static_cast<Core::i32>(env_lights.size());
  for (const auto& light : env_lights) {
    ubo_lights.at(i) = light;
    i++;
  }
  light_ubo.update();
};

auto
Renderer::generate_and_update_descriptor_write_sets(Material& material)
  -> VkDescriptorSet
{
  static std::array<IShaderBindable*, 8> structure_identifiers = {
    &renderer_ubo,
    &shadow_ubo,
    &point_light_ubo,
    &spot_light_ubo,
    &visible_point_lights_ssbo,
    &visible_spot_lights_ssbo,
    &screen_data_ubo,
    &directional_shadow_projections_ubo
  };
  const auto& shader = material.get_shader();

  static std::unordered_map<Core::usize, std::vector<VkWriteDescriptorSet>>
    shader_write_cache{};

  std::vector<VkWriteDescriptorSet>& write_descriptor_sets =
    shader_write_cache[shader->hash()];
  if (write_descriptor_sets.empty()) {
    write_descriptor_sets.reserve(structure_identifiers.size());
    for (const auto& identifier : structure_identifiers) {
      const auto* write = shader->get_descriptor_set(identifier->get_name(), 0);
      if (write == nullptr) {
        error("Failed to find descriptor set for identifier: {}",
              identifier->get_name());
        continue;
      }

      const auto* buffer_info = &identifier->get_descriptor_info();
      if (buffer_info == nullptr) {
        error("Failed to find buffer info for identifier: {}",
              identifier->get_name());
        continue;
      }

      VkWriteDescriptorSet descriptor_write{};
      descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptor_write.dstBinding = write->dstBinding;
      descriptor_write.dstArrayElement = 0;
      descriptor_write.descriptorType = write->descriptorType;
      descriptor_write.descriptorCount = 1;
      descriptor_write.pBufferInfo = buffer_info;
      write_descriptor_sets.push_back(descriptor_write);
    }
  }

  VkDescriptorSetAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  const auto& layouts = shader->get_descriptor_set_layouts();
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts = &layouts.at(0);
  auto* allocated =
    DescriptorResource::the().allocate_descriptor_set(alloc_info);

  for (auto& wds : write_descriptor_sets) {
    wds.dstSet = allocated;
  }

  vkUpdateDescriptorSets(Device::the().device(),
                         static_cast<Core::u32>(write_descriptor_sets.size()),
                         write_descriptor_sets.data(),
                         0,
                         nullptr);

  return allocated;
}

Renderer::Renderer(Configuration config, const Window* window)
  : size(window->get_swapchain().get_size())
  , old_size(size)
{
  Shader::initialise_compiler(Compilation::ShaderCompilerConfiguration{
    .optimisation_level = 2,
    .debug_information_level = Compilation::DebugInformationLevel::Full,
    .warnings_as_errors = false,
    .include_directories = { std::filesystem::path{ "shaders" } },
    .macro_definitions = {},
  });

  command_buffer = Core::make_scope<CommandBuffer>(CommandBuffer::Properties{
    .queue_type = QueueType::Graphics,
    .primary = true,
  });
  compute_command_buffer =
    Core::make_scope<CommandBuffer>(CommandBuffer::Properties{
      .queue_type = QueueType::Compute,
      .primary = true,
    });

  std::unordered_map<RendererTechnique, std::vector<std::string>>
    technique_construction_order;
  technique_construction_order[RendererTechnique::Deferred] = {
    "Shadow",   "Predepth",    "MainGeometry",
    "Deferred", "Lights",      "ChromaticAberration",
    "Bloom",    "Composition",
  };

  render_passes["MainGeometry"] =
    Core::make_scope<MainGeometryRenderPass>(*this);
  render_passes["Shadow"] =
    Core::make_scope<ShadowRenderPass>(*this, config.shadow_pass_size);
  render_passes["Deferred"] = Core::make_scope<DeferredRenderPass>(*this);
  render_passes["Predepth"] = Core::make_scope<PredepthRenderPass>(*this);
  render_passes["Lights"] = Core::make_scope<LightsRenderPass>(*this);
  render_passes["LightCulling"] =
    Core::make_scope<LightCullingRenderPass>(*this, light_culling_work_groups);
  render_passes["ChromaticAberration"] =
    Core::make_scope<ChromaticAberrationRenderPass>(*this);
  render_passes["Composition"] = Core::make_scope<CompositionRenderPass>(*this);
  render_passes["Bloom"] = Core::make_scope<BloomRenderPass>(*this);

  for (const auto& k :
       technique_construction_order.at(RendererTechnique::Deferred)) {
    render_passes.at(k)->construct();
  }

  technique_construction_order[RendererTechnique::ForwardPlus] = {
    "LightCulling"
  };
  for (const auto& k :
       technique_construction_order.at(RendererTechnique::ForwardPlus)) {
    render_passes.at(k)->construct();
  }

  activate_post_processing_step("Bloom");
  activate_post_processing_step("ChromaticAberration");
  activate_post_processing_step("Composition");

  transform_buffers.resize(3);
  static constexpr auto total_size = 100 * 1000 * sizeof(TransformVertexData);
  for (auto& [vertex_buffer, transform_buffer] : transform_buffers) {
    vertex_buffer = Core::make_scope<VertexBuffer>(total_size);
    transform_buffer = Core::make_scope<Core::DataBuffer>(total_size);
    transform_buffer->fill_zero();
  }

  Core::DataBuffer data_buffer{
    sizeof(Core::u32),
  };
  static constexpr auto white_data = 0xFFFFFFFF;
  data_buffer.write(&white_data, sizeof(Core::u32), 0U);

  white_texture = Image::load_from_memory(1,
                                          1,
                                          data_buffer,
                                          {
                                            .path = "white-default-texture",
                                          });

  Core::u32 black_data{ 0 };
  data_buffer.write(&black_data, sizeof(Core::u32), 0U);
  black_texture = Image::load_from_memory(
    1, 1, data_buffer, { .path = "black-default-texture" });

  const glm::uvec2 viewport_size{ size.width, size.height };

  constexpr uint32_t tile_size = 16U;
  glm::uvec2 vp_size = viewport_size;
  vp_size += tile_size - viewport_size % tile_size;
  light_culling_work_groups = { vp_size / tile_size, 1 };

  visible_point_lights_ssbo.resize(
    static_cast<Core::usize>(light_culling_work_groups.x *
                             light_culling_work_groups.y) *
    4 * 1024);
  visible_spot_lights_ssbo.resize(
    static_cast<Core::usize>(light_culling_work_groups.x *
                             light_culling_work_groups.y) *
    4 * 1024);

  struct
  {
    [[nodiscard]] static auto get_device() -> VkDevice
    {
      return Device::the().device();
    }
    [[nodiscard]] static auto get_queue_type() -> QueueType
    {
      return QueueType::Graphics;
    }
  } a{};
  thread_pool = Core::make_scope<ED::ThreadPool>(a, 4U);

  renderer_2d = Core::make_scope<Renderer2D>(*this, 1000U);
}

Renderer::~Renderer() = default;

auto
Renderer::destruct() -> void
{
  white_texture->destroy();
  black_texture->destroy();

  for (const auto& [k, v] : render_passes) {
    v->destruct();
  }

  command_buffer.reset();

  thread_pool.reset();
}

auto
Renderer::begin_scene(Core::Scene& scene,
                      const Core::SceneRendererCamera& camera) -> void
{
  if (old_size != size) {
    Device::the().wait();
    old_size = size;
    // We've been resized.

    auto& shadow_render_pass = get_render_pass("Shadow");
    auto& main_geom = get_render_pass("MainGeometry");
    auto& deferred = get_render_pass("Deferred");
    auto& predepth = get_render_pass("Predepth");
    auto& lights = get_render_pass("Lights");
    auto& chromatic_aberration = get_render_pass("ChromaticAberration");
    predepth.on_resize(size);
    shadow_render_pass.on_resize(size);
    main_geom.on_resize(size);
    deferred.on_resize(size);
    lights.on_resize(size);
    chromatic_aberration.on_resize(size);

    const glm::uvec2 viewport_size{ size.width, size.height };

    constexpr uint32_t tile_size = 16U;
    glm::uvec2 vp_size = viewport_size;
    vp_size += tile_size - viewport_size % tile_size;
    light_culling_work_groups = { vp_size / tile_size, 1 };

    visible_point_lights_ssbo.resize(light_culling_work_groups.x *
                                     light_culling_work_groups.y * 4 * 1024);
    visible_spot_lights_ssbo.resize(light_culling_work_groups.x *
                                    light_culling_work_groups.y * 4 * 1024);
  }

  const auto& light_environment = scene.get_light_environment();
  auto& [view,
         proj,
         view_proj,
         light_colour_intensity,
         specular_colour_intensity,
         ubo_cascade_splits,
         camera_pos] = renderer_ubo.get_data();
  view = camera.camera.get_view_matrix();
  proj = camera.camera.get_projection_matrix();
  view_proj = proj * view;
  camera_pos = camera.camera.get_position();
  light_colour_intensity = light_environment.colour_and_intensity;
  specular_colour_intensity = light_environment.specular_colour_and_intensity;
  compute_directional_shadow_projections(
    camera, glm::normalize(-light_environment.sun_position));
  ubo_cascade_splits = cascade_splits;
  renderer_ubo.update();

  auto& [light_view, light_proj, light_view_proj, light_pos, light_dir] =
    shadow_ubo.get_data();
  auto projection = light_environment.shadow_projection;
  auto view_matrix = glm::lookAt(glm::vec3{ light_environment.sun_position },
                                 glm::vec3(0.0F),
                                 glm::vec3(0.0F, 1.0F, 0.0F));
  light_view = view_matrix;
  light_proj = projection;
  light_view_proj = projection * view_matrix;
  light_pos = light_environment.sun_position;
  light_dir = glm::normalize(-light_environment.sun_position);
  shadow_ubo.update();

  update_lights(point_light_ubo, light_environment.point_lights);
  update_lights(spot_light_ubo, light_environment.spot_lights);

  // Visible spot lights
  auto& screen_data = screen_data_ubo.get_data();
  screen_data.full_resolution = glm::vec2{ size.width, size.height };
  screen_data.half_resolution = glm::vec2{ size.width / 2, size.height / 2 };
  screen_data.inv_resolution = glm::vec2{
    1.0F / static_cast<Core::f32>(size.width),
    1.0F / static_cast<Core::f32>(size.height),
  };

  float depth_linearize_mul = -projection[3][2];
  float depth_linearize_add = projection[2][2];
  if (depth_linearize_mul * depth_linearize_add < 0) {
    depth_linearize_add = -depth_linearize_add;
  }
  screen_data.depth_constants = { depth_linearize_mul, depth_linearize_add };
  screen_data.near_plane = camera.camera.get_near_clip();
  screen_data.far_plane = camera.camera.get_far_clip();
  screen_data.tile_count_x = light_culling_work_groups.x;
  static auto begin = Core::Clock::now();
  screen_data.time = static_cast<Core::f32>(Core::Clock::now() - begin);
  screen_data_ubo.update();
}

auto
Renderer::compute_directional_shadow_projections(
  const Core::SceneRendererCamera& camera,
  const glm::vec3& light_direction) -> void
{
  static Core::ShadowCascadeCalculator cascade_calculator{
    cascade_near_plane_offset,
    cascade_far_plane_offset,
  };
  auto cascades = cascade_calculator.compute_cascades(camera, light_direction);
  auto& [view_projections] = directional_shadow_projections_ubo.get_data();
  for (auto i = 0ULL; i < 4; i++) {
    cascade_splits[static_cast<Core::i32>(i)] = cascades[i].split_depth;
    view_projections[i] = cascades[i].view_projection;
  }
  directional_shadow_projections_ubo.update();
}

auto
Renderer::submit_static_mesh(Core::Ref<StaticMesh>& static_mesh,
                             const glm::mat4& transform) -> void
{
  const auto& source = static_mesh->get_mesh_asset();
  const auto& submesh_data = source->get_submeshes();
  for (const auto submesh_index : static_mesh->get_submeshes()) {
    glm::mat4 submesh_transform =
      transform * submesh_data[submesh_index].transform;

    const auto& vertex_buffer = source->get_vertex_buffer();
    const auto& index_buffer = source->get_index_buffer();
    const auto& material =
      source->get_materials().at(submesh_data[submesh_index].material_index);

    CommandKey key{
      &vertex_buffer, &index_buffer, material.get(), submesh_index
    };
    auto& mesh_transform = mesh_transform_map[key].transforms.emplace_back();
    mesh_transform.transform_rows[0] = {
      submesh_transform[0][0],
      submesh_transform[1][0],
      submesh_transform[2][0],
      submesh_transform[3][0],
    };
    mesh_transform.transform_rows[1] = {
      submesh_transform[0][1],
      submesh_transform[1][1],
      submesh_transform[2][1],
      submesh_transform[3][1],
    };
    mesh_transform.transform_rows[2] = {
      submesh_transform[0][2],
      submesh_transform[1][2],
      submesh_transform[2][2],
      submesh_transform[3][2],
    };

    auto& command = draw_commands[key];
    command.static_mesh = static_mesh;
    command.submesh_index = submesh_index;
    command.instance_count++;

    if (true /*mesh->casts_shadows()*/) {
      auto& shadow_command = shadow_draw_commands[key];
      shadow_command.static_mesh = static_mesh;
      shadow_command.submesh_index = submesh_index;
      shadow_command.instance_count++;
      // shadow_command.material = shadow_material.get();
    }
  }
}

auto
Renderer::submit_static_light(Core::Ref<StaticMesh>& static_mesh,
                              const glm::mat4& transform,
                              const glm::vec4& colour_times_intensity) -> void
{
  const auto& source = static_mesh->get_mesh_asset();
  const auto& submesh_data = source->get_submeshes();
  for (const auto submesh_index : static_mesh->get_submeshes()) {
    glm::mat4 submesh_transform =
      transform * submesh_data[submesh_index].transform;

    const auto& vertex_buffer = source->get_vertex_buffer();
    const auto& index_buffer = source->get_index_buffer();
    const auto& material =
      source->get_materials().at(submesh_data[submesh_index].material_index);

    CommandKey key{ &vertex_buffer, &index_buffer, material.get(), 0 };
    auto& mesh_transform = mesh_transform_map[key].transforms.emplace_back();
    mesh_transform.transform_rows[0] = {
      submesh_transform[0][0],
      submesh_transform[1][0],
      submesh_transform[2][0],
      submesh_transform[3][0],
    };
    mesh_transform.transform_rows[1] = {
      submesh_transform[0][1],
      submesh_transform[1][1],
      submesh_transform[2][1],
      submesh_transform[3][1],
    };
    mesh_transform.transform_rows[2] = {
      submesh_transform[0][2],
      submesh_transform[1][2],
      submesh_transform[2][2],
      submesh_transform[3][2],
    };

    auto& command = lights_draw_commands[key];
    command.static_mesh = static_mesh;
    command.submesh_index = submesh_index;
    command.instance_count++;
    lights_instance_data.emplace_back(colour_times_intensity);
  }
}

auto
Renderer::end_scene() -> void
{
  flush_draw_lists();
}

auto
Renderer::on_resize(const Core::Extent& new_size) -> void
{
  size = new_size;
}

auto
Renderer::flush_draw_lists() -> void
{
  Core::u32 offset = 0;
  const auto& [vb, tb] =
    transform_buffers.at(Core::Application::the().current_frame_index());

  for (auto& transform_data : mesh_transform_map | std::views::values) {
    transform_data.offset = offset * sizeof(TransformVertexData);
    for (const auto& transform : transform_data.transforms) {
      tb->write(&transform,
                sizeof(TransformVertexData),
                offset * sizeof(TransformVertexData));
      offset++;
    }
  }
  std::vector<TransformVertexData> output;
  output.resize(offset);

  tb->read(std::span{ output });
  vb->write(std::span{ output });

  command_buffer->begin();

  // Shadow pass
  render_passes.at("Shadow")->execute(*command_buffer);
  // Prepdepth pass
  render_passes.at("Predepth")->execute(*command_buffer);
  {
    compute_command_buffer->begin();
    render_passes.at("LightCulling")->execute(*compute_command_buffer);
    compute_command_buffer->end();
    compute_command_buffer->submit();
  }
  if (technique == RendererTechnique::Deferred) {
    // Geometry pass
    render_passes.at("MainGeometry")->execute(*command_buffer);
    // Deferred
    render_passes.at("Deferred")->execute(*command_buffer);

    render_passes.at("Lights")->execute(*command_buffer);
  } else if (technique == RendererTechnique::ForwardPlus) {
    render_passes.at("ForwardPlusGeometry")->execute(*command_buffer);
    render_passes.at("Composite")->execute(*command_buffer);
  }

  for (const auto& step : post_processing_steps) {
    if (!render_passes.contains(step.name)) {
      continue;
    }
    const auto& render_pass = render_passes.at(step.name);
    render_pass->execute(*command_buffer);
  }

  command_buffer->end();
  command_buffer->submit();

  draw_commands.clear();
  shadow_draw_commands.clear();
  lights_draw_commands.clear();
  mesh_transform_map.clear();
  lights_instance_data.clear();
}

auto
Renderer::screenshot() const -> void
{
  static auto index = 0ULL;
  Device::the().wait();
  const auto* image = get_final_output();
  if (image->write_to_file(
        std::format("Assets/images/screenshot-{}.png", index))) {
    info("Screenshot saved to screenshot.png");
    index++;
  } else {
    error("Failed to save screenshot to screenshot.png");
  }
}

auto
Renderer::get_shadow_output_image() const -> const Image*
{
  return render_passes.at("Shadow")
    ->get_extraneous_framebuffer(0)
    ->get_depth_attachment()
    .get();
}

[[nodiscard]] auto
Renderer::get_final_output() const -> const Image*
{
  if (!post_processing_steps.empty()) {
    return render_passes.at("Composition")
      ->get_framebuffer()
      ->get_colour_attachment(0)
      .get();
  }

  return render_passes.at("Lights")
    ->get_framebuffer()
    ->get_colour_attachment(0)
    .get();
}

} // namespace Engine::Graphics
