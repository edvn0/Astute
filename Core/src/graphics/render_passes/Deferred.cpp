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

static const Framebuffer* fb;

auto
Renderer::construct_deferred_pass(const Window* window,
                                  const Framebuffer& framebuffer) -> void
{
  auto& [deferred_framebuffer,
         deferred_shader,
         deferred_pipeline,
         deferred_material] = deferred_render_pass;
  deferred_framebuffer =
    Core::make_scope<Framebuffer>(Framebuffer::Configuration{
      .size = window->get_swapchain().get_size(),
      .colour_attachment_formats = { VK_FORMAT_R32G32B32A32_SFLOAT },
      .depth_attachment_format = VK_FORMAT_D32_SFLOAT,
      .sample_count = VK_SAMPLE_COUNT_4_BIT,
      .name = "Deferred",
    });
  deferred_shader = Shader::compile_graphics_scoped(
    "Assets/shaders/deferred.vert", "Assets/shaders/deferred.frag");
  deferred_pipeline =
    Core::make_scope<GraphicsPipeline>(GraphicsPipeline::Configuration{
      .framebuffer = deferred_framebuffer.get(),
      .shader = deferred_shader.get(),
      .sample_count = VK_SAMPLE_COUNT_4_BIT,
    });

  fb = &framebuffer;
}

auto
Renderer::deferred_pass() -> void
{
  const auto& [deferred_framebuffer,
               deferred_shader,
               deferred_pipeline,
               deferred_material] = deferred_render_pass;
  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = deferred_framebuffer->get_renderpass();
  render_pass_info.framebuffer = deferred_framebuffer->get_framebuffer();
  render_pass_info.renderArea.offset = { 0, 0 };
  render_pass_info.renderArea.extent = deferred_framebuffer->get_extent();
  const auto& clear_values = deferred_framebuffer->get_clear_values();
  render_pass_info.clearValueCount =
    static_cast<Core::u32>(clear_values.size());
  render_pass_info.pClearValues = clear_values.data();

  RendererExtensions::begin_renderpass(*command_buffer, *deferred_framebuffer);

  vkCmdBindPipeline(command_buffer->get_command_buffer(),
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    deferred_pipeline->get_pipeline());

  auto descriptor_set =
    generate_and_update_descriptor_write_sets(deferred_shader.get());

  VkWriteDescriptorSet write_descriptor_set_position{};
  write_descriptor_set_position.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_set_position.dstSet = descriptor_set;
  write_descriptor_set_position.dstBinding = 10;
  write_descriptor_set_position.dstArrayElement = 0;
  write_descriptor_set_position.descriptorType =
    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write_descriptor_set_position.descriptorCount = 1;
  write_descriptor_set_position.pImageInfo =
    &fb->get_colour_attachment(0)->get_descriptor_info();

  VkWriteDescriptorSet write_descriptor_set_normals{};
  write_descriptor_set_normals.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_set_normals.dstSet = descriptor_set;
  write_descriptor_set_normals.dstBinding = 11;
  write_descriptor_set_normals.dstArrayElement = 0;
  write_descriptor_set_normals.descriptorType =
    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write_descriptor_set_normals.descriptorCount = 1;
  write_descriptor_set_normals.pImageInfo =
    &fb->get_colour_attachment(1)->get_descriptor_info();

  VkWriteDescriptorSet write_descriptor_set_colour{};
  write_descriptor_set_colour.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_set_colour.dstSet = descriptor_set;
  write_descriptor_set_colour.dstBinding = 12;
  write_descriptor_set_colour.dstArrayElement = 0;
  write_descriptor_set_colour.descriptorType =
    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write_descriptor_set_colour.descriptorCount = 1;
  write_descriptor_set_colour.pImageInfo =
    &fb->get_colour_attachment(2)->get_descriptor_info();

  std::array write_sets = {
    write_descriptor_set_position,
    write_descriptor_set_normals,
    write_descriptor_set_colour,
  };

  vkUpdateDescriptorSets(Device::the().device(),
                         static_cast<Core::u32>(write_sets.size()),
                         write_sets.data(),
                         0,
                         nullptr);

  vkCmdBindDescriptorSets(command_buffer->get_command_buffer(),
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          deferred_pipeline->get_layout(),
                          0,
                          1,
                          &descriptor_set,
                          0,
                          nullptr);

  vkCmdDraw(command_buffer->get_command_buffer(), 3, 1, 0, 0);

  RendererExtensions::end_renderpass(*command_buffer);
}

} // namespace Engine::Graphics
