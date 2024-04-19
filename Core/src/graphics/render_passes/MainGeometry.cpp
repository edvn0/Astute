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

namespace Engine::Graphics {

auto
Renderer::construct_main_geometry_pass(const Window* window,
                                       Core::Ref<Image> predepth_attachment)
  -> void
{
  auto& [main_geometry_framebuffer,
         main_geometry_shader,
         main_geometry_pipeline,
         main_geometry_material] = main_geometry_render_pass;
  main_geometry_framebuffer =
    Core::make_scope<Framebuffer>(Framebuffer::Configuration{
      .size = window->get_swapchain().get_size(),
      .colour_attachment_formats = { VK_FORMAT_R32G32B32A32_SFLOAT,
                                     VK_FORMAT_R32G32B32A32_SFLOAT,
                                     VK_FORMAT_R32G32B32A32_SFLOAT, },
      .sample_count = VK_SAMPLE_COUNT_4_BIT,
      .dependent_attachments = {predepth_attachment},
      .name = "MainGeometry",
    });
  main_geometry_shader = Shader::compile_graphics_scoped(
    "Assets/shaders/main_geometry.vert", "Assets/shaders/main_geometry.frag");
  main_geometry_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = main_geometry_framebuffer.get(),
      .shader = main_geometry_shader.get(),
      .sample_count = VK_SAMPLE_COUNT_4_BIT,
      .cull_mode = VK_CULL_MODE_BACK_BIT,
      .face_mode = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    });
}

auto
Renderer::main_geometry_pass() -> void
{
  const auto& [main_geometry_framebuffer,
               main_geometry_shader,
               main_geometry_pipeline,
               main_geometry_material] = main_geometry_render_pass;
  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = main_geometry_framebuffer->get_renderpass();
  render_pass_info.framebuffer = main_geometry_framebuffer->get_framebuffer();
  render_pass_info.renderArea.offset = { 0, 0 };
  render_pass_info.renderArea.extent = main_geometry_framebuffer->get_extent();
  const auto& clear_values = main_geometry_framebuffer->get_clear_values();
  render_pass_info.clearValueCount =
    static_cast<Core::u32>(clear_values.size());
  render_pass_info.pClearValues = clear_values.data();

  RendererExtensions::begin_renderpass(*command_buffer,
                                       *main_geometry_framebuffer);

  vkCmdBindPipeline(command_buffer->get_command_buffer(),
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    main_geometry_pipeline->get_pipeline());
  auto descriptor_set =
    generate_and_update_descriptor_write_sets(main_geometry_shader.get());

  VkWriteDescriptorSet write_descriptor_set_image{};
  write_descriptor_set_image.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_set_image.dstSet = descriptor_set;
  write_descriptor_set_image.dstBinding = 10;
  write_descriptor_set_image.dstArrayElement = 0;
  write_descriptor_set_image.descriptorType =
    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write_descriptor_set_image.descriptorCount = 1;
  write_descriptor_set_image.pImageInfo =
    &shadow_render_pass.framebuffer->get_depth_attachment()
       ->get_descriptor_info();

  vkUpdateDescriptorSets(
    Device::the().device(), 1, &write_descriptor_set_image, 0, nullptr);

  vkCmdBindDescriptorSets(command_buffer->get_command_buffer(),
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          main_geometry_pipeline->get_layout(),
                          0,
                          1,
                          &descriptor_set,
                          0,
                          nullptr);

  for (const auto& [key, command] : draw_commands) {
    const auto& [vertex_buffer, index_buffer, submesh_index, instance_count] =
      command;

    auto vertex_buffers = std::array{ vertex_buffer->get_buffer() };
    auto offsets = std::array<VkDeviceSize, 1>{ 0 };
    vkCmdBindVertexBuffers(command_buffer->get_command_buffer(),
                           0,
                           1,
                           vertex_buffers.data(),
                           offsets.data());

    const auto& transform_vertex_buffer =
      transform_buffers.at(Core::Application::the().current_frame_index())
        .transform_buffer;
    auto offset = mesh_transform_map.at(key).offset;
    RendererExtensions::bind_vertex_buffer(
      *command_buffer, *transform_vertex_buffer, 1, offset);

    vkCmdBindIndexBuffer(command_buffer->get_command_buffer(),
                         index_buffer->get_buffer(),
                         0,
                         VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(command_buffer->get_command_buffer(),
                     static_cast<Core::u32>(index_buffer->count()),
                     instance_count,
                     0,
                     0,
                     0);
  }

  RendererExtensions::end_renderpass(*command_buffer);
}

} // namespace Engine::Graphics
