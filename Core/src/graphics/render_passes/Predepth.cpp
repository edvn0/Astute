#include "pch/CorePCH.hpp"

#include "graphics/Renderer.hpp"

#include "core/Logger.hpp"
#include "core/Scene.hpp"

#include "core/Application.hpp"
#include "graphics/DescriptorResource.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

#include "graphics/RendererExtensions.hpp"

namespace Engine::Graphics {

auto
Renderer::construct_predepth_pass(const Window* window) -> void
{
  auto& [predepth_framebuffer,
         predepth_shader,
         predepth_pipeline,
         predepth_material] = predepth_render_pass;
  predepth_framebuffer =
    Core::make_scope<Framebuffer>(Framebuffer::Configuration{
      .size = window->get_swapchain().get_size(),
      .depth_attachment_format = VK_FORMAT_D32_SFLOAT,
      .sample_count = VK_SAMPLE_COUNT_4_BIT,
      .name = "Predepth",
    });
  predepth_shader = Shader::compile_graphics_scoped(
    "Assets/shaders/predepth.vert", "Assets/shaders/predepth.frag");
  predepth_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = predepth_framebuffer.get(),
      .shader = predepth_shader.get(),
      .sample_count = VK_SAMPLE_COUNT_4_BIT,
    });
}

auto
Renderer::predepth_pass() -> void
{
  const auto& [predepth_framebuffer,
               predepth_shader,
               predepth_pipeline,
               predepth_material] = predepth_render_pass;
  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = predepth_framebuffer->get_renderpass();
  render_pass_info.framebuffer = predepth_framebuffer->get_framebuffer();
  render_pass_info.renderArea.offset = { 0, 0 };
  render_pass_info.renderArea.extent = predepth_framebuffer->get_extent();
  const auto& clear_values = predepth_framebuffer->get_clear_values();
  render_pass_info.clearValueCount =
    static_cast<Core::u32>(clear_values.size());
  render_pass_info.pClearValues = clear_values.data();

  RendererExtensions::begin_renderpass(*command_buffer, *predepth_framebuffer);

  vkCmdBindPipeline(command_buffer->get_command_buffer(),
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    predepth_pipeline->get_pipeline());
  auto descriptor_set =
    generate_and_update_descriptor_write_sets(predepth_shader.get());

  vkCmdBindDescriptorSets(command_buffer->get_command_buffer(),
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          predepth_pipeline->get_layout(),
                          0,
                          1,
                          &descriptor_set,
                          0,
                          nullptr);

  for (const auto& [key, command] : draw_commands) {
    const auto& [vertex_buffer,
                 index_buffer,
                 material,
                 submesh_index,
                 instance_count] = command;

    auto vertex_buffers = std::array{ vertex_buffer->get_buffer() };
    auto offsets = std::array<VkDeviceSize, 1>{ 0 };
    vkCmdBindVertexBuffers(command_buffer->get_command_buffer(),
                           0,
                           1,
                           vertex_buffers.data(),
                           offsets.data());

    const auto& transform_vertex_buffer =
      transform_buffers.at(Core::Application::the().current_frame_index())
        .transform_buffer;
    auto offset = mesh_transform_map.at(key).offset;
    RendererExtensions::bind_vertex_buffer(
      *command_buffer, *transform_vertex_buffer, 1, offset);

    vkCmdBindIndexBuffer(command_buffer->get_command_buffer(),
                         index_buffer->get_buffer(),
                         0,
                         VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(command_buffer->get_command_buffer(),
                     static_cast<Core::u32>(index_buffer->count()),
                     instance_count,
                     0,
                     0,
                     0);
  }

  RendererExtensions::end_renderpass(*command_buffer);
}

} // namespace Engine::Graphics
