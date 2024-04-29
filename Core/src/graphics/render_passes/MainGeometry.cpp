#include "pch/CorePCH.hpp"

#include "graphics/Renderer.hpp"

#include "core/Logger.hpp"
#include "core/Scene.hpp"

#include "core/Application.hpp"
#include "graphics/DescriptorResource.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/Image.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

#include "graphics/RendererExtensions.hpp"

#include "graphics/render_passes/MainGeometry.hpp"

namespace Engine::Graphics {

static Core::Ref<Image> normal_map;

auto
MainGeometryRenderPass::construct() -> void
{
  on_resize(get_renderer().get_size());
}

auto
MainGeometryRenderPass::execute_impl(CommandBuffer& command_buffer) -> void
{
  const auto& [main_geometry_framebuffer,
               main_geometry_shader,
               main_geometry_pipeline,
               main_geometry_material] = get_data();

  auto renderer_desc_set =
    generate_and_update_descriptor_write_sets(*main_geometry_material);

  for (auto&& [key, command] : get_renderer().draw_commands) {
    auto&& [mesh, submesh_index, instance_count] = command;

    const auto& mesh_asset = mesh->get_mesh_asset();
    const auto& transform_vertex_buffer =
      get_renderer()
        .transform_buffers.at(Core::Application::the().current_frame_index())
        .transform_buffer;
    auto offset = get_renderer().mesh_transform_map.at(key).offset;

    auto& material = mesh->get_materials().at(
      mesh_asset->get_submeshes().at(submesh_index).material_index);
    auto descriptor_set = material->generate_and_update_descriptor_write_sets();

    RendererExtensions::bind_vertex_buffer(
      command_buffer, mesh_asset->get_vertex_buffer(), 0);
    RendererExtensions::bind_vertex_buffer(
      command_buffer, *transform_vertex_buffer, 1, offset);
    RendererExtensions::bind_index_buffer(command_buffer,
                                          mesh_asset->get_index_buffer());

    std::array desc_sets{ renderer_desc_set, descriptor_set };
    vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            main_geometry_pipeline->get_layout(),
                            0,
                            static_cast<Core::u32>(desc_sets.size()),
                            desc_sets.data(),
                            0,
                            nullptr);
    const auto& push_constant_buffer = material->get_constant_buffer();

    if (push_constant_buffer) {
      vkCmdPushConstants(command_buffer.get_command_buffer(),
                         main_geometry_pipeline->get_layout(),
                         VK_SHADER_STAGE_ALL,
                         0,
                         push_constant_buffer.size(),
                         push_constant_buffer.raw());
    }

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
MainGeometryRenderPass::destruct_impl() -> void
{
  normal_map.reset();
}

auto
MainGeometryRenderPass::on_resize(const Core::Extent& ext) -> void
{
  RenderPass::for_each([&](const auto key, auto& val) {
    auto&& [main_geometry_framebuffer,
            main_geometry_shader,
            main_geometry_pipeline,
            main_geometry_material] = val;
    main_geometry_framebuffer =
    Core::make_scope<Framebuffer>(Framebuffer::Configuration{
      .size = ext,
      .colour_attachment_formats = {
          VK_FORMAT_R32G32B32A32_SFLOAT,
          VK_FORMAT_R32G32B32A32_SFLOAT,
          VK_FORMAT_R8G8B8A8_UNORM,
      },
      .depth_attachment_format = VK_FORMAT_D24_UNORM_S8_UINT,
      .sample_count = VK_SAMPLE_COUNT_4_BIT,
      .name = "MainGeometry",
    });
    main_geometry_shader = Shader::compile_graphics_scoped(
      "Assets/shaders/main_geometry.vert", "Assets/shaders/main_geometry.frag");
    main_geometry_pipeline =
      Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
        .framebuffer{ main_geometry_framebuffer.get() },
        .shader{ main_geometry_shader.get() },
        .sample_count{ VK_SAMPLE_COUNT_4_BIT },
        .cull_mode{ VK_CULL_MODE_BACK_BIT },
        .face_mode{ VK_FRONT_FACE_COUNTER_CLOCKWISE },
        .depth_comparator{ VK_COMPARE_OP_GREATER_OR_EQUAL },
      });
    main_geometry_material = Core::make_scope<Material>(Material::Configuration{
      .shader = main_geometry_shader.get(),
    });

    main_geometry_material->set(
      "shadow_map",
      get_renderer().get_render_pass("Shadow").get_depth_attachment());
  });
}

} // namespace Engine::Graphics
