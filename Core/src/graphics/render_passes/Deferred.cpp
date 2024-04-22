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
      .override_vertex_attributes = {
          {  },
        },
      .override_instance_attributes = {
          {  },
        },
    });

  deferred_material = Core::make_scope<Material>(Material::Configuration{
    .shader = deferred_shader.get(),
  });
  deferred_material->set("gPositionMap",
                         TextureType::Position,
                         framebuffer.get_colour_attachment(0));
  deferred_material->set(
    "gNormalMap", TextureType::Normal, framebuffer.get_colour_attachment(1));
  deferred_material->set("gAlbedoSpecMap",
                         TextureType::Albedo,
                         framebuffer.get_colour_attachment(2));

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
    generate_and_update_descriptor_write_sets(*deferred_material);

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
