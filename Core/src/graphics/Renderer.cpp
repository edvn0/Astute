#include "pch/CorePCH.hpp"

#include "graphics/Renderer.hpp"

#include "core/Application.hpp"
#include "core/Scene.hpp"
#include "logging/Logger.hpp"

#include "core/Clock.hpp"

#include "core/Random.hpp"
#include "graphics/DescriptorResource.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

#include "graphics/Framebuffer.hpp"

#include "graphics/RendererExtensions.hpp"
#include "graphics/TextureGenerator.hpp"

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
      auto write = shader->get_descriptor_set(identifier->get_name(), 0);
      if (!write) {
        error("Failed to find descriptor set for identifier: {}",
              identifier->get_name());
        continue;
      }

      auto* buffer_info = &identifier->get_descriptor_info();
      if (!buffer_info) {
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
  auto allocated =
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
    "Shadow", "Predepth", "MainGeometry", "Deferred", "Lights",
  };

  render_passes["MainGeometry"] =
    Core::make_scope<MainGeometryRenderPass>(*this);
  render_passes["Shadow"] =
    Core::make_scope<ShadowRenderPass>(*this, config.shadow_pass_size);
  render_passes["Deferred"] = Core::make_scope<DeferredRenderPass>(*this);
  render_passes["Predepth"] = Core::make_scope<PredepthRenderPass>(*this);
  render_passes["Lights"] = Core::make_scope<LightsRenderPass>(*this);

  for (const auto& k :
       technique_construction_order.at(RendererTechnique::Deferred)) {
    render_passes.at(k)->construct();
  }

  technique_construction_order[RendererTechnique::ForwardPlus] = {
    "LightCulling",
  };

  render_passes["LightCulling"] =
    Core::make_scope<LightCullingRenderPass>(*this, light_culling_work_groups);
  for (const auto& k :
       technique_construction_order.at(RendererTechnique::ForwardPlus)) {
    render_passes.at(k)->construct();
  }

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
    auto get_device() const -> VkDevice { return Device::the().device(); }
    auto get_queue_type() const -> QueueType { return QueueType::Graphics; }
  } a{};
  thread_pool = Core::make_scope<ED::ThreadPool>(a, 4U);
}

Renderer::~Renderer() = default;

auto
Renderer::destruct() -> void
{
  white_texture->destroy();
  black_texture->destroy();

  for (auto& [k, v] : render_passes) {
    v->destruct();
  }

  command_buffer.reset();

  thread_pool.reset();
}

auto
Renderer::begin_scene(Core::Scene& scene,
                      const SceneRendererCamera& camera) -> void
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
    predepth.on_resize(size);
    shadow_render_pass.on_resize(size);
    main_geom.on_resize(size);
    deferred.on_resize(size);
    lights.on_resize(size);

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
         camera_pos] = renderer_ubo.get_data();
  view = camera.camera.get_view_matrix();
  proj = camera.camera.get_projection_matrix();
  view_proj = proj * view;
  camera_pos = camera.camera.get_position();
  light_colour_intensity = light_environment.colour_and_intensity;
  specular_colour_intensity = light_environment.specular_colour_and_intensity;
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

  compute_directional_shadow_projections(camera, glm::vec3(light_dir));
}

auto
Renderer::compute_directional_shadow_projections(
  const SceneRendererCamera& camera,
  const glm::vec3& light_direction) -> void
{
  struct CascadeData
  {
    glm::mat4 view_projection;
    glm::mat4 view;
    Core::f32 split_depth;
  };
  std::array<CascadeData, 4> cascades{};

  auto view_projection =
    camera.camera.get_unreversed_projection_matrix() * camera.view_matrix;
  view_projection[1][1] *= -1.0f;

  static constexpr auto shadow_map_cascade_count = 4;
  static constexpr auto cascade_split_lambda = 0.95f;
  static constexpr auto CascadeFarPlaneOffset = 5.0f;
  static constexpr auto CascadeNearPlaneOffset = -5.0f;

  std::array<float, shadow_map_cascade_count> shadow_cascade_splits;

  float near_clip = camera.near;
  float far_clip = camera.far;
  float clip_range = far_clip - near_clip;

  float min_z = near_clip;
  float max_z = near_clip + clip_range;

  float range = max_z - min_z;
  float ratio = max_z / min_z;

  // Calculate split depths based on view camera frustum
  // Based on method presented in
  // https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
  for (uint32_t i = 0; i < shadow_map_cascade_count; i++) {
    float p = (static_cast<float>(i) + 1) /
              static_cast<float>(shadow_map_cascade_count);
    float log = min_z * std::pow(ratio, p);
    float uniform = min_z + range * p;
    float d = std::lerp(log, uniform, cascade_split_lambda);
    shadow_cascade_splits[i] = (d - near_clip) / clip_range;
  }

  shadow_cascade_splits[3] = 0.3f;

  // Manually set cascades here
  // shadow_cascade_splits[0] = 0.05f;
  // shadow_cascade_splits[1] = 0.15f;
  // shadow_cascade_splits[2] = 0.3f;
  // shadow_cascade_splits[3] = 1.0f;

  // Calculate orthographic projection matrix for each cascade
  float last_split_distance = 0.0;
  for (uint32_t k = 0; k < shadow_map_cascade_count; k++) {
    float splitDist = shadow_cascade_splits[k];

    std::array frustum_corners = {
      glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(1.0f, 1.0f, -1.0f),
      glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(-1.0f, -1.0f, -1.0f),
      glm::vec3(-1.0f, 1.0f, 1.0f),  glm::vec3(1.0f, 1.0f, 1.0f),
      glm::vec3(1.0f, -1.0f, 1.0f),  glm::vec3(-1.0f, -1.0f, 1.0f),
    };

    // Project frustum corners into world space
    glm::mat4 inverse_camera = glm::inverse(view_projection);
    for (auto& frustum_corner : frustum_corners) {
      glm::vec4 invCorner = inverse_camera * glm::vec4(frustum_corner, 1.0f);
      frustum_corner = invCorner / invCorner.w;
    }

    for (uint32_t i = 0; i < 4; i++) {
      glm::vec3 dist = frustum_corners[i + 4] - frustum_corners[i];
      frustum_corners[i + 4] = frustum_corners[i] + (dist * splitDist);
      frustum_corners[i] = frustum_corners[i] + (dist * last_split_distance);
    }

    // Get frustum center
    glm::vec3 frustum_center(0.0f);
    for (const auto& corner : frustum_corners)
      frustum_center += corner;

    frustum_center /= 8.0f;

    // frustum_center *= 0.01f;

    float radius = 0.0f;
    for (const auto& corner : frustum_corners) {
      float distance = glm::length(corner - frustum_center);
      radius = glm::max(radius, distance);
    }
    radius = std::ceil(radius * 16.0f) / 16.0f;

    glm::vec3 max_extents(radius);
    glm::vec3 min_extents = -max_extents;

    glm::vec3 light_dir = light_direction;
    glm::mat4 light_view_matrix =
      glm::lookAt(frustum_center - light_dir * -min_extents.z,
                  frustum_center,
                  glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 light_ortho_matrix =
      glm::ortho(min_extents.x,
                 max_extents.x,
                 min_extents.y,
                 max_extents.y,
                 0.0f + CascadeNearPlaneOffset,
                 max_extents.z - min_extents.z + CascadeFarPlaneOffset);

    // Offset to texel space to avoid shimmering (from
    // https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering)
    glm::mat4 shadowMatrix = light_ortho_matrix * light_view_matrix;
    float ShadowMapResolution = 4096.0F;
    glm::vec4 shadowOrigin =
      (shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) * ShadowMapResolution /
      2.0f;
    glm::vec4 rounded_origin = glm::round(shadowOrigin);
    glm::vec4 round_offset = rounded_origin - shadowOrigin;
    round_offset = round_offset * 2.0f / ShadowMapResolution;
    round_offset.z = 0.0f;
    round_offset.w = 0.0f;

    light_ortho_matrix[3] += round_offset;

    // Store split distance and matrix in cascade
    cascades[k].split_depth = (near_clip + splitDist * clip_range) * -1.0f;
    cascades[k].view_projection = light_ortho_matrix * light_view_matrix;
    cascades[k].view = light_view_matrix;

    last_split_distance = shadow_cascade_splits[k];
  }

  auto& [view_projections] = directional_shadow_projections_ubo.get_data();
  for (int i = 0; i < 4; i++) {
    cascade_splits[i] = cascades[i].split_depth;
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
                              const glm::mat4& transform) -> void
{

  const auto& source = static_mesh->get_mesh_asset();
  const auto& submesh_data = source->get_submeshes();
  for (const auto submesh_index : static_mesh->get_submeshes()) {
    glm::mat4 submesh_transform =
      transform * submesh_data[submesh_index].transform;

    const auto& vertex_buffer = source->get_vertex_buffer();
    const auto& index_buffer = source->get_index_buffer();
    auto& material =
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
    // TODO: Not yet implemented.
    render_passes.at("ForwardPlusGeometry")->execute(*command_buffer);
    render_passes.at("Composite")->execute(*command_buffer);
  }

  command_buffer->end();
  command_buffer->submit();

  draw_commands.clear();
  shadow_draw_commands.clear();
  lights_draw_commands.clear();
  mesh_transform_map.clear();
}

auto
Renderer::screenshot() -> void
{
  static auto index = 0ULL;
  Device::the().wait();
  auto image = get_final_output();
  if (image->write_to_file(std::format("screenshot-{}.png", index))) {
    info("Screenshot saved to screenshot.png");
  } else {
    error("Failed to save screenshot to screenshot.png");
  }
}

} // namespace Engine::Graphics
