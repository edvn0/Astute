#include "pch/CorePCH.hpp"

#include "graphics/render_passes/Deferred.hpp"

#include "core/Logger.hpp"
#include "core/Scene.hpp"

#include "core/Application.hpp"
#include "graphics/DescriptorResource.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/GraphicsPipeline.hpp"
#include "graphics/Image.hpp"
#include "graphics/Material.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/Shader.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

#include "graphics/RendererExtensions.hpp"

namespace Engine::Graphics {

auto
DeferredRenderPass::construct(Renderer& renderer) -> void
{
  auto&& [deferred_framebuffer,
          deferred_shader,
          deferred_pipeline,
          deferred_material] = get_data();
  deferred_framebuffer =
    Core::make_scope<Framebuffer>(Framebuffer::Configuration{
      .size = renderer.get_size(),
      .colour_attachment_formats = { VK_FORMAT_R32G32B32A32_SFLOAT },
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

  auto& input_render_pass = renderer.get_render_pass("MainGeometry");
  deferred_material->set("gPositionMap",
                         TextureType::Position,
                         input_render_pass.get_colour_attachment(0));
  deferred_material->set("gNormalMap",
                         TextureType::Normal,
                         input_render_pass.get_colour_attachment(1));
  deferred_material->set("gAlbedoSpecMap",
                         TextureType::Albedo,
                         input_render_pass.get_colour_attachment(2));
}

auto
DeferredRenderPass::execute_impl(Renderer& renderer,
                                 CommandBuffer& command_buffer) -> void
{
  const auto&& [deferred_framebuffer,
                deferred_shader,
                deferred_pipeline,
                deferred_material] = get_data();

  auto descriptor_set =
    generate_and_update_descriptor_write_sets(renderer, *deferred_material);

  vkCmdBindDescriptorSets(command_buffer.get_command_buffer(),
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          deferred_pipeline->get_layout(),
                          0,
                          1,
                          &descriptor_set,
                          0,
                          nullptr);

  vkCmdDraw(command_buffer.get_command_buffer(), 3, 1, 0, 0);
}

auto
DeferredRenderPass::destruct_impl() -> void
{
}

} // namespace Engine::Graphics
