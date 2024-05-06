#include "pch/CorePCH.hpp"

#include "graphics/render_passes/Predepth.hpp"

#include "core/Logger.hpp"
#include "core/Scene.hpp"

#include "core/Application.hpp"
#include "graphics/DescriptorResource.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/GraphicsPipeline.hpp"
#include "graphics/Image.hpp"
#include "graphics/Material.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/Shader.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

#include "graphics/RendererExtensions.hpp"

namespace Engine::Graphics {

auto
PredepthRenderPass::construct() -> void
{
  on_resize(get_renderer().get_size());
}

auto
PredepthRenderPass::execute_impl(CommandBuffer& command_buffer) -> void
{
  const auto& [predepth_framebuffer,
               predepth_shader,
               predepth_pipeline,
               predepth_material] = get_data();

  auto descriptor_set =
    generate_and_update_descriptor_write_sets(*predepth_material);

  vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          predepth_pipeline->get_layout(),
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

  for (const auto& [key, command] : get_renderer().draw_commands) {
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
PredepthRenderPass::destruct_impl() -> void
{
}

auto
PredepthRenderPass::on_resize(const Core::Extent& ext) -> void
{
  auto&& [predepth_framebuffer,
          predepth_shader,
          predepth_pipeline,
          predepth_material] = get_data();
  predepth_framebuffer =
    Core::make_scope<Framebuffer>(Framebuffer::Configuration{
      .size = get_renderer().get_size(),
      .colour_attachment_formats = {},
      .depth_attachment_format = VK_FORMAT_D32_SFLOAT,
      .sample_count = VK_SAMPLE_COUNT_4_BIT,
      .immediate_construction = false,
      .name = "Predepth",
    });
  predepth_framebuffer->create_framebuffer_fully();

  predepth_shader = Shader::compile_graphics_scoped(
    "Assets/shaders/predepth.vert", "Assets/shaders/empty.frag");
  predepth_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = predepth_framebuffer.get(),
      .shader = predepth_shader.get(),
      .sample_count = VK_SAMPLE_COUNT_4_BIT,
      .cull_mode = VK_CULL_MODE_BACK_BIT,
      .face_mode = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depth_comparator = VK_COMPARE_OP_GREATER,
      .override_vertex_attributes = { {
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
      } },
    });

  predepth_material = Core::make_scope<Material>(Material::Configuration{
    .shader = predepth_shader.get(),
  });
}

} // namespace Engine::Graphics
