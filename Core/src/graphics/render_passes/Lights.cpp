#include "pch/CorePCH.hpp"

#include "graphics/Renderer.hpp"

#include "core/Application.hpp"
#include "core/Logger.hpp"
#include "core/Scene.hpp"

#include "graphics/DescriptorResource.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

#include "graphics/RendererExtensions.hpp"

#include "graphics/render_passes/Lights.hpp"

namespace Engine::Graphics {

auto
LightsRenderPass::construct() -> void
{
  auto&& [lights_framebuffer, lights_shader, lights_pipeline, lights_material] =
    get_data();
  lights_framebuffer = Core::make_scope<Framebuffer>(FramebufferSpecification{
    .width = get_renderer().get_size().width,
    .height = get_renderer().get_size().height,
    .clear_colour_on_load = false,
    .clear_depth_on_load = false,
    .attachments = { { VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_D32_SFLOAT } },
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .existing_images = { {0, get_renderer().get_render_pass("Deferred").get_colour_attachment(0),}, { 1, get_renderer()
                             .get_render_pass("Predepth")
                             .get_depth_attachment(), }, },
    .debug_name = "Lights",
  });
  lights_shader = Shader::compile_graphics_scoped(
    "Assets/shaders/main_geometry.vert", "Assets/shaders/lights.frag");
  lights_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = lights_framebuffer.get(),
      .shader = lights_shader.get(),
      .sample_count = VK_SAMPLE_COUNT_1_BIT,
      .cull_mode = VK_CULL_MODE_BACK_BIT,
      .face_mode = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depth_comparator = VK_COMPARE_OP_GREATER_OR_EQUAL,
    });

  lights_material = Core::make_scope<Material>(Material::Configuration{
    .shader = lights_shader.get(),
  });
}

auto
LightsRenderPass::execute_impl(CommandBuffer& command_buffer) -> void
{
  const auto& [lights_framebuffer,
               lights_shader,
               lights_pipeline,
               lights_material] = get_data();

  auto descriptor_set =
    generate_and_update_descriptor_write_sets(*lights_material);

  vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                          lights_pipeline->get_bind_point(),
                          lights_pipeline->get_layout(),
                          0,
                          1,
                          &descriptor_set,
                          0,
                          nullptr);

  for (const auto& [key, command] : get_renderer().lights_draw_commands) {
    const auto& [mesh, submesh_index, instance_count] = command;

    const auto& mesh_asset = mesh->get_mesh_asset();
    auto vertex_buffers =
      std::array{ mesh_asset->get_vertex_buffer().get_buffer() };
    auto offsets = std::array<VkDeviceSize, 1>{ 0 };
    vkCmdBindVertexBuffers(command_buffer.get_command_buffer(),
                           0,
                           1,
                           vertex_buffers.data(),
                           offsets.data());

    const auto& transform_vertex_buffer =
      get_renderer()
        .transform_buffers.at(Core::Application::the().current_frame_index())
        .transform_buffer;
    auto vb = transform_vertex_buffer->get_buffer();
    auto offset = get_renderer().mesh_transform_map.at(key).offset;
    const auto& submesh = mesh_asset->get_submeshes().at(submesh_index);

    offsets = std::array{ VkDeviceSize{ offset } };

    vkCmdBindVertexBuffers(
      command_buffer.get_command_buffer(), 1, 1, &vb, offsets.data());

    vkCmdBindIndexBuffer(command_buffer.get_command_buffer(),
                         mesh_asset->get_index_buffer().get_buffer(),
                         0,
                         VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(command_buffer.get_command_buffer(),
                     submesh.index_count,
                     instance_count,
                     submesh.base_index,
                     submesh.base_vertex,
                     0);
  }
}

auto
LightsRenderPass::on_resize(const Core::Extent& ext) -> void
{
  auto&& [framebuffer, shader, pipeline, material] = get_data();
  framebuffer->on_resize(ext);
  pipeline->on_resize(ext);
}

} // namespace Engine::Graphics
