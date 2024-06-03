#include "pch/CorePCH.hpp"

#include "graphics/Renderer.hpp"
#include "logging/Logger.hpp"

#include "core/Application.hpp"
#include "core/Maths.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/RendererExtensions.hpp"

#include <ranges>
#include <span>

#include "graphics/GraphicsPipeline.hpp"

#include "graphics/render_passes/Shadow.hpp"

namespace Engine::Graphics {

static constexpr auto create_layer_views = [](auto& image) {
  auto sequence = Core::monotone_sequence<4>();
  image->create_specific_layer_image_views(std::span{ sequence });
};

auto
ShadowRenderPass::construct_impl() -> void
{
  auto&& [_, shadow_shader, __, shadow_material] = get_data();
  static constexpr auto shadow_map_count = 4ULL;
  cascaded_shadow_map = Core::make_ref<Image>(ImageConfiguration{
    .width = size,
    .height = size,
    .mip_levels = 1,
    .layers = shadow_map_count,
    .format = VK_FORMAT_D32_SFLOAT,
    .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    .usage =
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    .additional_name_data = "Cascaded Shadow Map",
  });
  create_layer_views(cascaded_shadow_map);
  FramebufferSpecification spec{
    .width = size,
    .height = size,
    .depth_clear_value = 1.0F,
    .clear_depth_on_load = true,
    .attachments = { { VK_FORMAT_D32_SFLOAT } },
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .no_resize = true,
    .existing_image = cascaded_shadow_map,
    .existing_image_layers = { 0 },
    .debug_name = "Shadow",
  };
  shadow_shader = Shader::compile_graphics_scoped("Assets/shaders/shadow.vert",
                                                  "Assets/shaders/empty.frag");
  shadow_material = Core::make_scope<Material>(Material::Configuration{
    .shader = shadow_shader.get(),
  });

  for (const auto i : std::views::iota(0ULL, shadow_map_count)) {
    spec.existing_image_layers.clear();
    spec.existing_image_layers.push_back(static_cast<Core::i32>(i));
    other_framebuffers.push_back(Core::make_scope<Framebuffer>(spec));
    other_pipelines.push_back(
      Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
        .framebuffer = other_framebuffers.back().get(),
        .shader = shadow_shader.get(),
        .depth_comparator = VK_COMPARE_OP_LESS,
        .override_vertex_attributes = { {
          {
            0,
            0,
            VK_FORMAT_R32G32B32_SFLOAT,
            0,
          },
        }, },
      }));
  }
}

auto
ShadowRenderPass::execute_impl(CommandBuffer& command_buffer) -> void
{
  ASTUTE_PROFILE_FUNCTION();

  const auto& [_, shadow_shader, __, shadow_material] = get_data();

  auto* descriptor_set =
    generate_and_update_descriptor_write_sets(*shadow_material);

  static auto perform_pass = [&](const Core::DataBuffer& cascade_buffer,
                                 const IPipeline& pipeline) {
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
      auto* vb = transform_vertex_buffer->get_buffer();
      auto offset = get_renderer().mesh_transform_map.at(key).offset;
      const auto& submesh = mesh_asset->get_submeshes().at(submesh_index);

      offsets = std::array{ VkDeviceSize{ offset } };
      vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                              pipeline.get_bind_point(),
                              pipeline.get_layout(),
                              0,
                              1,
                              &descriptor_set,
                              0,
                              nullptr);
      vkCmdPushConstants(command_buffer.get_command_buffer(),
                         pipeline.get_layout(),
                         VK_SHADER_STAGE_ALL,
                         0,
                         cascade_buffer.size_u32(),
                         cascade_buffer.raw());

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
  };

  static Core::DataBuffer current_cascade_buffer{ sizeof(Core::u32) };
  current_cascade_buffer.fill_zero();
  for (const auto i : std::views::iota(0ULL, other_framebuffers.size())) {
    ASTUTE_PROFILE_SCOPE("Shadow Render Pass Cascade Number: " +
                         std::to_string(i));
    current_cascade_buffer.write(&i, sizeof(Core::u32));
    RendererExtensions::begin_renderpass(command_buffer,
                                         *other_framebuffers.at(i));
    RendererExtensions::bind_pipeline(command_buffer, *other_pipelines.at(i));
    perform_pass(current_cascade_buffer, *other_pipelines.at(i));
    RendererExtensions::end_renderpass(command_buffer);
  }
}

auto
ShadowRenderPass::on_resize(const Core::Extent& ext) -> void
{
  for (const auto& other : other_framebuffers) {
    other->on_resize(ext);
  }

  for (const auto& other : other_pipelines) {
    other->on_resize(ext);
  }
}

} // namespace Engine::Graphics
