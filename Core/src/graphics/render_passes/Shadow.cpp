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
ShadowRenderPass::construct() -> void
{
  RenderPass::for_each([&](const auto key, auto& val) {
    auto&& [shadow_framebuffer,
            shadow_shader,
            shadow_pipeline,
            shadow_material] = val;
    shadow_framebuffer =
    Core::make_scope<Framebuffer>(Framebuffer::Configuration{
      .size = {
        size,
        size,
      },
      .depth_attachment_format = VK_FORMAT_D32_SFLOAT,
      .sample_count = VK_SAMPLE_COUNT_1_BIT,
      .resizable = false,
      .depth_clear_value = 1,
      .name = "Shadow",
    });
    shadow_shader = Shader::compile_graphics_scoped(
      "Assets/shaders/shadow.vert", "Assets/shaders/shadow.frag");
    shadow_pipeline =
      Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
        .framebuffer = shadow_framebuffer.get(),
        .shader = shadow_shader.get(),
        .sample_count = VK_SAMPLE_COUNT_1_BIT,
        .cull_mode = VK_CULL_MODE_BACK_BIT,
        .face_mode = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depth_comparator = VK_COMPARE_OP_LESS_OR_EQUAL,
        .override_vertex_attributes = { {
          { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
        } },
      });

    shadow_material = Core::make_scope<Material>(Material::Configuration{
      .shader = shadow_shader.get(),
    });
  });
}

auto
ShadowRenderPass::execute_impl(CommandBuffer& command_buffer) -> void
{
  const auto& [shadow_framebuffer,
               shadow_shader,
               shadow_pipeline,
               shadow_material] = get_data();

  auto descriptor_set =
    generate_and_update_descriptor_write_sets(*shadow_material);

  vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          shadow_pipeline->get_layout(),
                          0,
                          1,
                          &descriptor_set,
                          0,
                          nullptr);

  static constexpr float depthBiasConstant = 1.25f;
  static constexpr float depthBiasSlope = 1.75f;

  vkCmdSetDepthBias(command_buffer.get_command_buffer(),
                    depthBiasConstant,
                    0.0f,
                    depthBiasSlope);

  for (const auto& [key, command] : get_renderer().shadow_draw_commands) {
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

    offsets = std::array{ VkDeviceSize{ offset } };

    vkCmdBindVertexBuffers(
      command_buffer.get_command_buffer(), 1, 1, &vb, offsets.data());

    vkCmdBindIndexBuffer(command_buffer.get_command_buffer(),
                         mesh_asset->get_index_buffer().get_buffer(),
                         0,
                         VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(
      command_buffer.get_command_buffer(),
      static_cast<Core::u32>(mesh_asset->get_index_buffer().count()),
      instance_count,
      0,
      0,
      0);
  }
}

auto
ShadowRenderPass::on_resize(const Core::Extent& ext) -> void
{
  RenderPass::for_each([&](const auto key, auto& val) {
    auto&& [framebuffer, shader, pipeline, material] = val;
    framebuffer->on_resize(ext);
    pipeline->on_resize(ext);
  });
}

} // namespace Engine::Graphics
