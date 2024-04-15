#include "pch/CorePCH.hpp"

#include "graphics/Renderer.hpp"

#include "core/Scene.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

namespace Engine::Graphics {

Renderer::Renderer(Configuration config, const Window* window)
  : size(window->get_swapchain().get_size())
  , shadow_pass_parameters({ config.shadow_pass_size })
{
}

auto
Renderer::begin_scene(Core::Scene& scene, SceneRendererCamera camera) -> void
{
}

auto
Renderer::end_scene() -> void
{
  flush_draw_lists();
}

auto
Renderer::predepth_pass() -> void
{
  VkRenderPassBeginInfo render_pass_info{};
  /*render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = predepth_framebuffer->get_renderpass();
  render_pass_info.framebuffer = predepth_framebuffer->get_framebuffer();
  render_pass_info.renderArea.offset = { 0, 0 };
  render_pass_info.renderArea.extent = predepth_framebuffer->get_extent();
  const auto& clear_values = predepth_framebuffer->get_clear_values();
  render_pass_info.clearValueCount =
    static_cast<Core::u32>(clear_values.size());
  render_pass_info.pClearValues = clear_values.data();

  vkCmdBeginRenderPass(command_buffer->get_command_buffer(),
                       &render_pass_info,
                       VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer->get_command_buffer(),
    VK_PIPELINE_BIND_POINT_GRAPHICS, predepth_pipeline->get_pipeline());
    */
}

auto
Renderer::flush_draw_lists() -> void
{
  // Predepth pass
  predepth_pass();

  // Shadow pass
  // Geometry pass
}

} // namespace Engine::Graphics
