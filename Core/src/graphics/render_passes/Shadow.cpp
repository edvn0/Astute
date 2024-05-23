#include "pch/CorePCH.hpp"

#include "graphics/Renderer.hpp"

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
  auto&& [shadow_framebuffer, shadow_shader, shadow_pipeline, shadow_material] =
    get_data();
  cascaded_shadow_map = Core::make_ref<Image>(ImageConfiguration{
    .width = size,
    .height = size,
    .mip_levels = 1,
    .layers = 4,
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
    .clear_depth_on_load = true,
    .attachments = { { VK_FORMAT_D32_SFLOAT } },
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .no_resize = true,
    .existing_image = cascaded_shadow_map,
    .existing_image_layers = { 0 },
    .debug_name = "Shadow",
  };
  shadow_framebuffer = Core::make_scope<Framebuffer>(spec);

  shadow_shader = Shader::compile_graphics_scoped("Assets/shaders/shadow.vert",
                                                  "Assets/shaders/empty.frag");
  shadow_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = shadow_framebuffer.get(),
      .shader = shadow_shader.get(),
      .sample_count = VK_SAMPLE_COUNT_1_BIT,
      .depth_comparator = VK_COMPARE_OP_GREATER,
      .override_vertex_attributes = { {
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
      } },
    });

  {
    spec.existing_image_layers.clear();
    spec.existing_image_layers.push_back(1);
    auto& other = other_framebuffers.emplace_back();
    other = Core::make_scope<Framebuffer>(spec);
    auto& other_pipeline = other_pipelines.emplace_back();
    other_pipeline =
      Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
        .framebuffer = other.get(),
        .shader = shadow_shader.get(),
        .sample_count = VK_SAMPLE_COUNT_1_BIT,
        .depth_comparator = VK_COMPARE_OP_GREATER,
        .override_vertex_attributes = { {
          { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
        } },
      });
  }

  {
    spec.existing_image_layers.clear();
    spec.existing_image_layers.push_back(2);
    auto& other = other_framebuffers.emplace_back();
    other = Core::make_scope<Framebuffer>(spec);

    auto& other_pipeline = other_pipelines.emplace_back();
    other_pipeline =
      Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
        .framebuffer = other.get(),
        .shader = shadow_shader.get(),
        .sample_count = VK_SAMPLE_COUNT_1_BIT,
        .depth_comparator = VK_COMPARE_OP_GREATER,
        .override_vertex_attributes = { {
          { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
        } },
      });
  }

  {
    spec.existing_image_layers.clear();
    spec.existing_image_layers.push_back(3);
    auto& other = other_framebuffers.emplace_back();
    other = Core::make_scope<Framebuffer>(spec);
    auto& other_pipeline = other_pipelines.emplace_back();
    other_pipeline =
      Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
        .framebuffer = other.get(),
        .shader = shadow_shader.get(),
        .sample_count = VK_SAMPLE_COUNT_1_BIT,
        .depth_comparator = VK_COMPARE_OP_GREATER,
        .override_vertex_attributes = { {
          { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
        } },
      });
  }

  shadow_material = Core::make_scope<Material>(Material::Configuration{
    .shader = shadow_shader.get(),
  });
}

auto
ShadowRenderPass::execute_impl(CommandBuffer& command_buffer) -> void
{
  ASTUTE_PROFILE_FUNCTION();

  const auto& [shadow_framebuffer,
               shadow_shader,
               shadow_pipeline,
               shadow_material] = get_data();

  auto descriptor_set =
    generate_and_update_descriptor_write_sets(*shadow_material);

  vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                          shadow_pipeline->get_bind_point(),
                          shadow_pipeline->get_layout(),
                          0,
                          1,
                          &descriptor_set,
                          0,
                          nullptr);
  static auto perform_pass = [&](const Core::DataBuffer& buffer) {
    for (const auto& [key, command] : get_renderer().shadow_draw_commands) {
      ASTUTE_PROFILE_SCOPE("Shadow Draw Command");
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

      vkCmdPushConstants(command_buffer.get_command_buffer(),
                         shadow_pipeline->get_layout(),
                         VK_SHADER_STAGE_ALL,
                         0,
                         buffer.size_u32(),
                         buffer.raw());

      vkCmdDrawIndexed(command_buffer.get_command_buffer(),
                       submesh.index_count,
                       instance_count,
                       submesh.base_index,
                       submesh.base_vertex,
                       0);
    }
  };

  RendererExtensions::begin_renderpass(command_buffer, *shadow_framebuffer);
  RendererExtensions::bind_pipeline(command_buffer, *shadow_pipeline);
  Core::DataBuffer current_cascade_buffer{ sizeof(Core::u32) };
  current_cascade_buffer.fill_zero();
  perform_pass(current_cascade_buffer);
  RendererExtensions::end_renderpass(command_buffer);

  for (const auto i : std::views::iota(0ULL, other_framebuffers.size())) {
    RendererExtensions::begin_renderpass(command_buffer,
                                         *other_framebuffers.at(i));
    RendererExtensions::bind_pipeline(command_buffer, *other_pipelines.at(i));
    current_cascade_buffer.write(&i, sizeof(Core::u32));
    perform_pass(current_cascade_buffer);
    RendererExtensions::end_renderpass(command_buffer);
  }
}

auto
ShadowRenderPass::on_resize(const Core::Extent& ext) -> void
{
  auto&& [fb, _, pipe, __] = get_data();

  fb->on_resize(ext);
  pipe->on_resize(ext);

  for (const auto& other : other_framebuffers) {
    other->on_resize(ext);
  }

  for (const auto& other : other_pipelines) {
    other->on_resize(ext);
  }
}

} // namespace Engine::Graphics
