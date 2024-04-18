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
Renderer::generate_and_update_descriptor_write_sets(const Shader* shader)
  -> VkDescriptorSet
{
  static std::array<IShaderBindable*, 2> structure_identifiers = {
    &renderer_ubo,
    &shadow_ubo,
  };

  VkDescriptorSetAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  const auto& layouts = shader->get_descriptor_set_layouts();
  alloc_info.descriptorSetCount = static_cast<Core::u32>(layouts.size());
  alloc_info.pSetLayouts = layouts.data();
  auto allocated =
    DescriptorResource::the().allocate_descriptor_set(alloc_info);

  std::vector<VkWriteDescriptorSet> write_descriptor_sets;
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

    write_descriptor_sets.emplace_back(VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                       nullptr,
                                       allocated,
                                       write->dstBinding,
                                       0,
                                       write->descriptorCount,
                                       write->descriptorType,
                                       nullptr,
                                       buffer_info,
                                       nullptr);
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
{
  Shader::initialise_compiler(Compilation::ShaderCompilerConfiguration{
    .optimisation_level = 0,
    .debug_information_level = Compilation::DebugInformationLevel::None,
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
  construct_shadow_pass(window, config.shadow_pass_size);

  transform_buffers.resize(3);
  static constexpr auto total_size = 100 * 1000 * sizeof(TransformVertexData);
  for (auto& [vertex_buffer, transform_buffer] : transform_buffers) {
    vertex_buffer = Core::make_scope<
      UniformBufferObject<TransformMapData, GPUBufferType::Vertex>>(
      total_size, "TransformBuffer");
    transform_buffer = Core::make_scope<Core::DataBuffer>(total_size);
    transform_buffer->fill_zero();
  }
}

Renderer::~Renderer()
{
  command_buffer.reset();
}

auto
Renderer::begin_scene(Core::Scene& scene, const SceneRendererCamera& camera)
  -> void
{
  auto& [view, proj, view_proj, camera_pos] = renderer_ubo.get_data();
  view = camera.camera.get_view_matrix();
  proj = camera.camera.get_projection_matrix();
  view_proj = proj * view;
  camera_pos = camera.camera.get_position();
  renderer_ubo.update();

  const auto& light_environment = scene.get_light_environment();
  auto& [light_view, light_proj, light_view_proj, light_pos] =
    shadow_ubo.get_data();
  auto projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 20.0f);
  auto view_matrix = glm::lookAt(
    light_environment.position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  light_view = view_matrix;
  light_proj = projection;
  light_view_proj = projection * view_matrix;
  light_pos = light_environment.position;
  shadow_ubo.update();
}

auto
Renderer::submit_static_mesh(const Graphics::VertexBuffer& vertex_buffer,
                             const Graphics::IndexBuffer& index_buffer,
                             const glm::mat4& transform,
                             const glm::vec4& tint) -> void
{
  CommandKey key{ &vertex_buffer, &index_buffer, 0 };

  auto& mesh_transform = mesh_transform_map[key].transforms.emplace_back();
  mesh_transform.transform_rows[0] = {
    transform[0][0], transform[1][0], transform[2][0], transform[3][0]
  };
  mesh_transform.transform_rows[1] = {
    transform[0][1], transform[1][1], transform[2][1], transform[3][1]
  };
  mesh_transform.transform_rows[2] = {
    transform[0][2], transform[1][2], transform[2][2], transform[3][2]
  };
  // We also instance the colour :)
  mesh_transform.transform_rows[3] = tint;

  auto& command = draw_commands[key];
  command.vertex_buffer = &vertex_buffer;
  command.index_buffer = &index_buffer;
  command.submesh_index = 0;
  command.instance_count++;
  // command.material = mesh->get_material(submesh);

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
  predepth_render_pass.framebuffer->on_resize(new_size);
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
  command_buffer->end();
  command_buffer->submit();

  draw_commands.clear();
  shadow_draw_commands.clear();
  mesh_transform_map.clear();
}

} // namespace Engine::Graphics
