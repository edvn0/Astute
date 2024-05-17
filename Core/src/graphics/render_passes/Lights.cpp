#include "pch/CorePCH.hpp"

#include "graphics/render_passes/Lights.hpp"

#include "core/Application.hpp"

#include "graphics/Framebuffer.hpp"
#include "graphics/GraphicsPipeline.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/RendererExtensions.hpp"

namespace Engine::Graphics {

auto
LightsRenderPass::construct() -> void
{
  auto&& [lights_framebuffer, lights_shader, lights_pipeline, lights_material] =
    get_data();
  lights_framebuffer = Core::make_scope<Framebuffer>(FramebufferSpecification{
    .width = get_renderer().get_size().width,
    .height = get_renderer().get_size().height,
    .clear_colour_on_load = false,
    .clear_depth_on_load = false,
    .attachments = { { VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_D32_SFLOAT } },
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .existing_images = { {
                           0,
                           get_renderer()
                             .get_render_pass("Deferred")
                             .get_colour_attachment(0),
                         },
                         {
                           1,
                           get_renderer()
                             .get_render_pass("Predepth")
                             .get_depth_attachment(),
                         } },
    .debug_name = "Lights",
  });
  lights_shader = Shader::compile_graphics_scoped("Assets/shaders/lights.vert",
                                                  "Assets/shaders/lights.frag");
  lights_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = lights_framebuffer.get(),
      .shader = lights_shader.get(),
      .sample_count = VK_SAMPLE_COUNT_1_BIT,
      .cull_mode = VK_CULL_MODE_FRONT_BIT,
      .face_mode = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    });

  lights_material = Core::make_scope<Material>(Material::Configuration{
    .shader = lights_shader.get(),
  });

  lights_material->set(
    "predepth_map",
    get_renderer().get_render_pass("Predepth").get_depth_attachment());
}

auto
LightsRenderPass::execute_impl(CommandBuffer& command_buffer) -> void
{
  const auto& [lights_framebuffer,
               lights_shader,
               lights_pipeline,
               lights_material] = get_data();

  auto* renderer_desc_set =
    generate_and_update_descriptor_write_sets(*lights_material);

  lights_material->update_descriptor_write_sets(renderer_desc_set);

  for (auto&& [key, command] : get_renderer().lights_draw_commands) {
    auto&& [mesh, submesh_index, instance_count] = command;

    const auto& mesh_asset = mesh->get_mesh_asset();
    const auto& transform_vertex_buffer =
      get_renderer()
        .transform_buffers.at(Core::Application::the().current_frame_index())
        .transform_buffer;
    auto offset = get_renderer().mesh_transform_map.at(key).offset;
    const auto& submesh = mesh_asset->get_submeshes().at(submesh_index);

    const auto& material = mesh->get_materials().at(submesh.material_index);
    auto* material_descriptor_set =
      material->generate_and_update_descriptor_write_sets();

    RendererExtensions::bind_vertex_buffer(
      command_buffer, mesh_asset->get_vertex_buffer(), 0);
    RendererExtensions::bind_vertex_buffer(
      command_buffer, *transform_vertex_buffer, 1, offset);
    RendererExtensions::bind_index_buffer(command_buffer,
                                          mesh_asset->get_index_buffer());

    std::array desc_sets{ renderer_desc_set, material_descriptor_set };
    vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                            lights_pipeline->get_bind_point(),
                            lights_pipeline->get_layout(),
                            0,
                            static_cast<Core::u32>(desc_sets.size()),
                            desc_sets.data(),
                            0,
                            nullptr);

    if (const auto& push_constant_buffer = material->get_constant_buffer();
        push_constant_buffer) {
      vkCmdPushConstants(command_buffer.get_command_buffer(),
                         lights_pipeline->get_layout(),
                         VK_SHADER_STAGE_ALL,
                         0,
                         static_cast<Core::u32>(push_constant_buffer.size()),
                         push_constant_buffer.raw());
    }

    vkCmdDrawIndexed(command_buffer.get_command_buffer(),
                     submesh.index_count,
                     instance_count,
                     submesh.base_index,
                     submesh.base_vertex,
                     0);
  }
}

auto
LightsRenderPass::on_resize(const Core::Extent& ext) -> void
{
  auto&& [fb, _, pipe, __] = get_data();

  fb->on_resize(ext);
  pipe->on_resize(ext);
}

} // namespace Engine::Graphics
