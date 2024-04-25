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

#include "graphics/render_passes/PreDepth.hpp"

namespace Engine::Graphics {

auto
PreDepthRenderPass::construct(Renderer& renderer) -> void
{
  auto&& [predepth_framebuffer,
          predepth_shader,
          predepth_pipeline,
          predepth_material] = get_data();
  predepth_framebuffer =
    Core::make_scope<Framebuffer>(Framebuffer::Configuration{
      .size = renderer.get_size(),
      .depth_attachment_format = VK_FORMAT_D24_UNORM_S8_UINT,
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
      .override_vertex_attributes =
        {
          { { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 } },
        },
    });

  predepth_material = Core::make_scope<Material>(Material::Configuration{
    .shader = predepth_shader.get(),
  });
}

auto
PreDepthRenderPass::execute_impl(Renderer& renderer,
                                 CommandBuffer& command_buffer) -> void
{
  const auto&& [predepth_framebuffer,
                predepth_shader,
                predepth_pipeline,
                predepth_material] = get_data();

  auto descriptor_set =
    generate_and_update_descriptor_write_sets(renderer, *predepth_material);

  vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          predepth_pipeline->get_layout(),
                          0,
                          1,
                          &descriptor_set,
                          0,
                          nullptr);

  for (const auto& [key, command] : renderer.draw_commands) {
    const auto& [vertex_buffer,
                 index_buffer,
                 material,
                 submesh_index,
                 instance_count] = command;

    auto vertex_buffers = std::array{ vertex_buffer->get_buffer() };
    auto offsets = std::array<VkDeviceSize, 1>{ 0 };
    vkCmdBindVertexBuffers(command_buffer.get_command_buffer(),
                           0,
                           1,
                           vertex_buffers.data(),
                           offsets.data());

    const auto& transform_vertex_buffer =
      renderer.transform_buffers
        .at(Core::Application::the().current_frame_index())
        .transform_buffer;
    auto offset = renderer.mesh_transform_map.at(key).offset;
    RendererExtensions::bind_vertex_buffer(
      command_buffer, *transform_vertex_buffer, 1, offset);

    vkCmdBindIndexBuffer(command_buffer.get_command_buffer(),
                         index_buffer->get_buffer(),
                         0,
                         VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(command_buffer.get_command_buffer(),
                     static_cast<Core::u32>(index_buffer->count()),
                     instance_count,
                     0,
                     0,
                     0);
  }
}

} // namespace Engine::Graphics
