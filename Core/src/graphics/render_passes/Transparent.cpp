#include "pch/CorePCH.hpp"

#include "graphics/Renderer.hpp"

#include "core/Clock.hpp"
#include "core/Profiler.hpp"
#include "core/Scene.hpp"
#include "core/Verify.hpp"
#include "logging/Logger.hpp"

#include "core/Application.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GraphicsPipeline.hpp"

#include "graphics/RendererExtensions.hpp"

#include <ranges>
#include <vulkan/vulkan_core.h>

#include "graphics/render_passes/Transparent.hpp"

namespace Engine::Graphics {

auto
TransparentRenderPass::construct_impl() -> void
{
  const auto& ext = get_renderer().get_size();
  auto&& [transparent_framebuffer,
          transparent_shader,
          transparent_pipeline,
          transparent_material] = get_data();
  transparent_framebuffer =
    Core::make_scope<Framebuffer>(FramebufferSpecification{
      .width = ext.width,
      .height= ext.height,
      .clear_colour_on_load = false,
      .clear_depth_on_load = false,
      .attachments = {
          { .format = VK_FORMAT_R32G32B32A32_SFLOAT, }, // deferred
          { .format = VK_FORMAT_D32_SFLOAT, }, // depth
      },
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .blend_mode = FramebufferBlendMode::SrcAlphaOneMinusSrcAlpha,
      .existing_images = {
            {0, get_renderer().get_render_pass("Deferred").get_colour_attachment(0), },
            {1, get_renderer().get_render_pass("Predepth").get_depth_attachment(), },
        },
      .debug_name = "Transparent",
    });
  transparent_shader =
    Shader::compile_graphics_scoped(Core::shaders_file("transparent.vert"),
                                    Core::shaders_file("transparent.frag"));
  transparent_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = transparent_framebuffer.get(),
      .shader = transparent_shader.get(),
      .sample_count = VK_SAMPLE_COUNT_1_BIT,
      .depth_comparator = VK_COMPARE_OP_EQUAL,
    });
  transparent_material = Core::make_scope<Material>(Material::Configuration{
    .shader = transparent_shader.get(),
  });

  depth_attachment =
    get_renderer().get_render_pass("Shadow").get_depth_attachment();
}

auto
TransparentRenderPass::execute_impl(CommandBuffer& command_buffer) -> void
{

  ASTUTE_PROFILE_FUNCTION();
  const auto& [transparent_framebuffer,
               transparent_shader,
               transparent_pipeline,
               transparent_material] = get_data();
  auto* renderer_desc_set =
    generate_and_update_descriptor_write_sets(*transparent_material);

  for (const auto& [key, command] : get_renderer().transparent_draw_commands) {
    ASTUTE_PROFILE_SCOPE("Transparent draw command");
    const auto& [mesh, submesh_index, instance_count] = command;

    const auto& mesh_asset = mesh->get_mesh_asset();
    const auto& transform_vertex_buffer =
      get_renderer()
        .transform_buffers.at(Core::Application::the().current_frame_index())
        .transform_buffer;
    auto offset = get_renderer().mesh_transform_map.at(key).offset;
    const auto& submesh = mesh_asset->get_submeshes().at(submesh_index);
    const auto& material = mesh->get_materials().at(submesh.material_index);
    material->set("shadow_map", depth_attachment);
    auto* material_descriptor_set =
      material->generate_and_update_descriptor_write_sets();

    RendererExtensions::bind_vertex_buffer(
      command_buffer, mesh_asset->get_vertex_buffer(), 0);
    RendererExtensions::bind_vertex_buffer(
      command_buffer, *transform_vertex_buffer, 1, offset);
    RendererExtensions::bind_index_buffer(command_buffer,
                                          mesh_asset->get_index_buffer());

    std::array desc_sets{ renderer_desc_set, material_descriptor_set };
    RendererExtensions::bind_descriptor_sets(
      command_buffer, *transparent_pipeline, std::span{ desc_sets });

    if (const auto& push_constant_buffer = material->get_constant_buffer();
        push_constant_buffer) {
      vkCmdPushConstants(command_buffer.get_command_buffer(),
                         transparent_pipeline->get_layout(),
                         VK_SHADER_STAGE_ALL,
                         0,
                         static_cast<Core::u32>(push_constant_buffer.size()),
                         push_constant_buffer.raw());
    }

    vkCmdDrawIndexed(command_buffer.get_command_buffer(),
                     submesh.index_count,
                     instance_count,
                     submesh.base_index,
                     static_cast<Core::i32>(submesh.base_vertex),
                     0);
  }
}

auto
TransparentRenderPass::destruct_impl() -> void
{
}

auto
TransparentRenderPass::on_resize(const Core::Extent& ext) -> void
{
  auto&& [fb, _, pipe, __] = get_data();

  fb->on_resize(ext);
  pipe->on_resize(ext);
}

} // namespace Engine::Graphics
