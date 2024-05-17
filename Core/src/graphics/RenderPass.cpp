#include "pch/CorePCH.hpp"

#include "graphics/RenderPass.hpp"

#include "graphics/Framebuffer.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/RendererExtensions.hpp"

namespace Engine::Graphics {

auto
RenderPass::execute(CommandBuffer& command_buffer, bool is_compute) -> void
{
#ifdef ASTUTE_DEBUG
  if (!is_valid()) {
    return;
  }
#endif

  bind(command_buffer);
  if (is_compute) {
    execute_compute_impl(command_buffer);
  } else {
    execute_impl(command_buffer);
  }
  unbind(command_buffer);
}

auto
RenderPass::get_colour_attachment(Core::u32 index) const
  -> const Core::Ref<Image>&
{
  return get_framebuffer()->get_colour_attachment(index);
}

auto
RenderPass::get_depth_attachment() const -> const Core::Ref<Image>&
{
  return get_framebuffer()->get_depth_attachment();
}

auto
RenderPass::bind(CommandBuffer& command_buffer) -> void
{
  const auto& pipeline = std::get<Core::Scope<IPipeline>>(pass);
  if (pipeline->get_bind_point() == VK_PIPELINE_BIND_POINT_GRAPHICS) {
    RendererExtensions::begin_renderpass(command_buffer, *get_framebuffer());
  }

  vkCmdBindPipeline(command_buffer.get_command_buffer(),
                    pipeline->get_bind_point(),
                    pipeline->get_pipeline());
}

auto
RenderPass::unbind(CommandBuffer& command_buffer) -> void
{
  const auto& pipeline = std::get<Core::Scope<IPipeline>>(pass);
  if (pipeline->get_bind_point() == VK_PIPELINE_BIND_POINT_GRAPHICS) {
    RendererExtensions::end_renderpass(command_buffer);
  }
}

auto
RenderPass::generate_and_update_descriptor_write_sets(Material& material)
  -> VkDescriptorSet
{
  return renderer.generate_and_update_descriptor_write_sets(material);
}

auto
RenderPass::blit_to(const CommandBuffer&, const Framebuffer&, BlitProperties)
  -> void
{
}

} // namespace Engine::Graphic
