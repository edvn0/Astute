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
MainGeometryRenderPass::construct(Renderer& renderer) -> void
{
  auto&& [main_geometry_framebuffer,
          main_geometry_shader,
          main_geometry_pipeline,
          main_geometry_material] = get_data();
  main_geometry_framebuffer =
    Core::make_scope<Framebuffer>(Framebuffer::Configuration{
      .size = renderer.get_size(),
      .colour_attachment_formats = { VK_FORMAT_R32G32B32A32_SFLOAT, // position
                                     VK_FORMAT_R32G32B32A32_SFLOAT, // normals
                                     VK_FORMAT_R32G32B32A32_SFLOAT, // albedo + spec
                                     },
      .sample_count = VK_SAMPLE_COUNT_4_BIT,
      .dependent_attachments = {
        renderer.get_render_pass("PreDepth").get_depth_attachment(),
      },
      .name = "MainGeometry",
    });
  main_geometry_shader = Shader::compile_graphics_scoped(
    "Assets/shaders/main_geometry.vert", "Assets/shaders/main_geometry.frag");
  main_geometry_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = main_geometry_framebuffer.get(),
      .shader = main_geometry_shader.get(),
      .sample_count = VK_SAMPLE_COUNT_4_BIT,
    });
  main_geometry_material = Core::make_scope<Material>(Material::Configuration{
    .shader = main_geometry_shader.get(),
  });

  main_geometry_material->set(
    "shadow_map",
    TextureType::Shadow,
    renderer.get_render_pass("Shadow").get_depth_attachment());

  normal_map = Graphics::Image::load_from_file("Assets/images/cube_normal.png");
}

auto
MainGeometryRenderPass::execute_impl(Renderer& renderer,
                                     CommandBuffer& command_buffer) -> void
{
  const auto&& [main_geometry_framebuffer,
                main_geometry_shader,
                main_geometry_pipeline,
                main_geometry_material] = get_data();

  auto descriptor_set = generate_and_update_descriptor_write_sets(
    renderer, *main_geometry_material);

  for (const auto& [key, command] : renderer.draw_commands) {
    const auto& [vertex_buffer,
                 index_buffer,
                 material,
                 submesh_index,
                 instance_count] = command;
    const auto& transform_vertex_buffer =
      renderer.transform_buffers
        .at(Core::Application::the().current_frame_index())
        .transform_buffer;
    auto offset = renderer.mesh_transform_map.at(key).offset;

    if (material) {
      material->set("normal_map", Graphics::TextureType::Normal, normal_map);
      material->generate_and_update_descriptor_write_sets(descriptor_set);
    }

    RendererExtensions::bind_vertex_buffer(command_buffer, *vertex_buffer, 0);
    RendererExtensions::bind_vertex_buffer(
      command_buffer, *transform_vertex_buffer, 1, offset);
    RendererExtensions::bind_index_buffer(command_buffer, *index_buffer);

    vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            main_geometry_pipeline->get_layout(),
                            0,
                            1,
                            &descriptor_set,
                            0,
                            nullptr);
    vkCmdDrawIndexed(command_buffer.get_command_buffer(),
                     static_cast<Core::u32>(index_buffer->count()),
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

} // namespace Engine::Graphics
