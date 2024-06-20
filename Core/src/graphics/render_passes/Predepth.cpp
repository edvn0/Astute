#include "pch/CorePCH.hpp"

#include "graphics/render_passes/Predepth.hpp"

#include "core/Application.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/GraphicsPipeline.hpp"
#include "graphics/Material.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/Shader.hpp"

#include "graphics/RendererExtensions.hpp"

namespace Engine::Graphics {

auto
PredepthRenderPass::construct_impl() -> void
{
  ASTUTE_PROFILE_FUNCTION();

  const auto& ext = get_renderer().get_size();
  auto&& [predepth_framebuffer,
          predepth_shader,
          predepth_pipeline,
          predepth_material] = get_data();
  predepth_framebuffer = Core::make_scope<Framebuffer>(FramebufferSpecification{
    .width = ext.width,
    .height = ext.height,
    .clear_depth_on_load = false,
    .attachments = { { .format = VK_FORMAT_D32_SFLOAT } },
    .debug_name = "Predepth",
  });

  predepth_shader = Shader::compile_graphics_scoped(
    "Assets/shaders/predepth.vert", "Assets/shaders/empty.frag");
  predepth_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = predepth_framebuffer.get(),
      .shader = predepth_shader.get(),
      .sample_count = VK_SAMPLE_COUNT_1_BIT,
      .depth_comparator = VK_COMPARE_OP_GREATER,
      .override_vertex_attributes = { {
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
      } },
    });

  predepth_material = Core::make_scope<Material>(Material::Configuration{
    .shader = predepth_shader.get(),
  });
}

auto
PredepthRenderPass::execute_impl(CommandBuffer& command_buffer) -> void
{
  ASTUTE_PROFILE_FUNCTION();
  const auto& [predepth_framebuffer,
               predepth_shader,
               predepth_pipeline,
               predepth_material] = get_data();
  RendererExtensions::explicitly_clear_framebuffer(command_buffer,
                                                   *predepth_framebuffer);

  auto* descriptor_set =
    generate_and_update_descriptor_write_sets(*predepth_material);

  vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                          predepth_pipeline->get_bind_point(),
                          predepth_pipeline->get_layout(),
                          0,
                          1,
                          &descriptor_set,
                          0,
                          nullptr);

  static constexpr float depth_bias_constant = 1.25F;
  static constexpr float depth_bias_slope = 1.75F;

  vkCmdSetDepthBias(command_buffer.get_command_buffer(),
                    depth_bias_constant,
                    0.0F,
                    depth_bias_slope);

  for (const auto& [key, command] : get_renderer().draw_commands) {
    ASTUTE_PROFILE_SCOPE("Predepth Draw Command");
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
    auto* vb = transform_vertex_buffer->get_buffer();
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
                     static_cast<Core::i32>(submesh.base_vertex),
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
  auto&& [fb, _, pipe, __] = get_data();

  fb->on_resize(ext);
  pipe->on_resize(ext);
}

} // namespace Engine::Graphics
