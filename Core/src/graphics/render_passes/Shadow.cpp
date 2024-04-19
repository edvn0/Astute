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

namespace Engine::Graphics {

auto
Renderer::construct_shadow_pass(const Window* window,
                                Core::u32 shadow_pass_size) -> void
{
  auto& [shadow_framebuffer, shadow_shader, shadow_pipeline, shadow_material] =
    shadow_render_pass;
  shadow_framebuffer =
    Core::make_scope<Framebuffer>(Framebuffer::Configuration{
      .size = {
        shadow_pass_size,
        shadow_pass_size,
      },
      .depth_attachment_format = VK_FORMAT_D32_SFLOAT,
      .sample_count = VK_SAMPLE_COUNT_1_BIT,
      .resizable = false,
      .name = "Shadow",
    });
  shadow_shader = Shader::compile_graphics_scoped("Assets/shaders/shadow.vert",
                                                  "Assets/shaders/shadow.frag");
  shadow_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = shadow_framebuffer.get(),
      .shader = shadow_shader.get(),
    });
}

auto
Renderer::shadow_pass() -> void
{
  const auto& [shadow_framebuffer,
               shadow_shader,
               shadow_pipeline,
               shadow_material] = shadow_render_pass;
  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = shadow_framebuffer->get_renderpass();
  render_pass_info.framebuffer = shadow_framebuffer->get_framebuffer();
  render_pass_info.renderArea.offset = { 0, 0 };
  render_pass_info.renderArea.extent = shadow_framebuffer->get_extent();
  const auto& clear_values = shadow_framebuffer->get_clear_values();
  render_pass_info.clearValueCount =
    static_cast<Core::u32>(clear_values.size());
  render_pass_info.pClearValues = clear_values.data();

  RendererExtensions::begin_renderpass(*command_buffer, *shadow_framebuffer);

  vkCmdBindPipeline(command_buffer->get_command_buffer(),
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    shadow_pipeline->get_pipeline());
  auto descriptor_set =
    generate_and_update_descriptor_write_sets(shadow_shader.get());

  vkCmdBindDescriptorSets(command_buffer->get_command_buffer(),
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          shadow_pipeline->get_layout(),
                          0,
                          1,
                          &descriptor_set,
                          0,
                          nullptr);

  for (const auto& [key, command] : shadow_draw_commands) {
    const auto& [vertex_buffer, index_buffer, submesh_index, instance_count] =
      command;

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
    auto vb = transform_vertex_buffer->get_buffer();
    auto offset = mesh_transform_map.at(key).offset;

    offsets = std::array{ VkDeviceSize{ offset } };

    vkCmdBindVertexBuffers(
      command_buffer->get_command_buffer(), 1, 1, &vb, offsets.data());

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
