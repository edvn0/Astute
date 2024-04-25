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

#include "graphics/render_passes/Shadow.hpp"

namespace Engine::Graphics {

auto
ShadowRenderPass::construct(Renderer& renderer) -> void
{
  auto&& [shadow_framebuffer, shadow_shader, shadow_pipeline, shadow_material] =
    get_data();
  shadow_framebuffer =
    Core::make_scope<Framebuffer>(Framebuffer::Configuration{
      .size = {
        size,
        size,
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
      .override_vertex_attributes = { {
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
      } },
    });

  shadow_material = Core::make_scope<Material>(Material::Configuration{
    .shader = shadow_shader.get(),
  });
}

auto
ShadowRenderPass::execute_impl(Renderer& renderer,
                               CommandBuffer& command_buffer) -> void
{
  const auto&& [shadow_framebuffer,
                shadow_shader,
                shadow_pipeline,
                shadow_material] = get_data();

  auto descriptor_set =
    generate_and_update_descriptor_write_sets(renderer, *shadow_material);

  vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          shadow_pipeline->get_layout(),
                          0,
                          1,
                          &descriptor_set,
                          0,
                          nullptr);

  for (const auto& [key, command] : renderer.shadow_draw_commands) {
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
    auto vb = transform_vertex_buffer->get_buffer();
    auto offset = renderer.mesh_transform_map.at(key).offset;

    offsets = std::array{ VkDeviceSize{ offset } };

    vkCmdBindVertexBuffers(
      command_buffer.get_command_buffer(), 1, 1, &vb, offsets.data());

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
