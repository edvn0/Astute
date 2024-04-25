#include "pch/CorePCH.hpp"

#include "graphics/Renderer.hpp"

#include "core/Application.hpp"
#include "core/Logger.hpp"
#include "core/Scene.hpp"

#include "graphics/DescriptorResource.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

#include "graphics/RendererExtensions.hpp"

#include <ranges>
#include <span>

namespace Engine::Graphics {

auto
Renderer::generate_and_update_descriptor_write_sets(Material& material)
  -> VkDescriptorSet
{
  static std::array<IShaderBindable*, 4> structure_identifiers = {
    &renderer_ubo,
    &shadow_ubo,
    &point_light_ubo,
    &spot_light_ubo,
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
  alloc_info.descriptorSetCount = static_cast<Core::u32>(layouts.size());
  alloc_info.pSetLayouts = layouts.data();
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

  // Material specific writes now.
  material.generate_and_update_descriptor_write_sets(allocated);

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
    .image_count = window->get_swapchain().get_image_count(),
    .queue_type = QueueType::Graphics,
    .primary = true,
  });

  construct_predepth_pass(window);
  construct_main_geometry_pass(
    window, predepth_render_pass.framebuffer->get_depth_attachment());
  construct_shadow_pass(window, config.shadow_pass_size);
  construct_deferred_pass(window, *main_geometry_render_pass.framebuffer);

  transform_buffers.resize(3);
  static constexpr auto total_size = 100 * 1000 * sizeof(TransformVertexData);
  for (auto& [vertex_buffer, transform_buffer] : transform_buffers) {
    vertex_buffer = Core::make_scope<VertexBuffer>(total_size);
    transform_buffer = Core::make_scope<Core::DataBuffer>(total_size);
    transform_buffer->fill_zero();
  }

  Core::DataBuffer data_buffer{ sizeof(Core::u32) };
  static constexpr auto white_data = 0xFFFFFFFF;
  data_buffer.write(&white_data, sizeof(Core::u32), 0U);

  white_texture = Image::load_from_memory(1, 1, data_buffer);

  Core::u32 black_data{ 0 };
  data_buffer.write(&black_data, sizeof(Core::u32), 0U);
  black_texture = Image::load_from_memory(1, 1, data_buffer);
}

Renderer::~Renderer() = default;

auto
Renderer::destruct() -> void
{
  white_texture->destroy();
  black_texture->destroy();

  predepth_render_pass.destruct();
  main_geometry_render_pass.destruct();
  shadow_render_pass.destruct();
  deferred_render_pass.destruct();

  command_buffer.reset();
}

auto
Renderer::begin_scene(Core::Scene& scene, const SceneRendererCamera& camera)
  -> void
{
  if (old_size != size) {
    // We've been resized.
    shadow_render_pass.on_resize(size);
    /*  predepth_render_pass.on_resize(size);

      main_geometry_render_pass.on_resize(size);
      main_geometry_render_pass.update_dependent_attachment(
        0, predepth_render_pass.framebuffer->get_depth_attachment());
      main_geometry_render_pass.material->set(
        "shadow_map",
        TextureType::Shadow,
        shadow_render_pass.framebuffer->get_depth_attachment());

      deferred_render_pass.on_resize(size);
      deferred_render_pass.material->set(
        "gPositionMap",
        TextureType::Position,
        main_geometry_render_pass.framebuffer->get_colour_attachment(0));
      deferred_render_pass.material->set(
        "gNormalMap",
        TextureType::Normal,
        main_geometry_render_pass.framebuffer->get_colour_attachment(1));
      deferred_render_pass.material->set(
        "gAlbedoSpecMap",
        TextureType::Albedo,
        main_geometry_render_pass.framebuffer->get_colour_attachment(2));
  */
    old_size = size;
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

  auto& [light_view, light_proj, light_view_proj, light_pos] =
    shadow_ubo.get_data();
  auto projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1F, 20.0F);
  auto view_matrix = glm::lookAt(light_environment.sun_position,
                                 glm::vec3(0.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f));
  light_view = view_matrix;
  light_proj = projection;
  light_view_proj = projection * view_matrix;
  light_pos = light_environment.sun_position;
  shadow_ubo.update();

  static constexpr auto update_lights = []<class Light>(Light& light_ubo,
                                                        auto& env_lights) {
    auto i = 0ULL;
    auto& [count, lights] = light_ubo.get_data();
    count = static_cast<Core::i32>(env_lights.size());
    for (const auto& light : env_lights) {
      lights.at(i) = light;
      i++;
    }
    light_ubo.update();
  };

  update_lights(point_light_ubo, light_environment.point_lights);
  update_lights(spot_light_ubo, light_environment.spot_lights);
}

auto
Renderer::submit_static_mesh(const Graphics::VertexBuffer& vertex_buffer,
                             const Graphics::IndexBuffer& index_buffer,
                             Graphics::Material& material,
                             const glm::mat4& transform) -> void
{
  CommandKey key{ &vertex_buffer, &index_buffer, &material, 0 };

  auto& mesh_transform = mesh_transform_map[key].transforms.emplace_back();
  mesh_transform.transform_rows[0] = {
    transform[0][0],
    transform[1][0],
    transform[2][0],
    transform[3][0],
  };
  mesh_transform.transform_rows[1] = {
    transform[0][1],
    transform[1][1],
    transform[2][1],
    transform[3][1],
  };
  mesh_transform.transform_rows[2] = {
    transform[0][2],
    transform[1][2],
    transform[2][2],
    transform[3][2],
  };

  auto& command = draw_commands[key];
  command.vertex_buffer = &vertex_buffer;
  command.index_buffer = &index_buffer;
  command.material = &material;
  command.submesh_index = 0;
  command.instance_count++;

  if (true /*mesh->casts_shadows()*/) {
    auto& shadow_command = shadow_draw_commands[key];
    shadow_command.vertex_buffer = &vertex_buffer;
    shadow_command.index_buffer = &index_buffer;
    shadow_command.submesh_index = 0;
    shadow_command.instance_count++;
    // shadow_command.material = shadow_material.get();
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

  // Predepth pass
  predepth_pass();
  // Shadow pass
  shadow_pass();
  // Geometry pass
  main_geometry_pass();
  // Deferred
  deferred_pass();

  command_buffer->end();
  command_buffer->submit();

  draw_commands.clear();
  shadow_draw_commands.clear();
  mesh_transform_map.clear();
}

} // namespace Engine::Graphics
