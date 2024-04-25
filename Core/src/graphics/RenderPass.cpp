#include "pch/CorePCH.hpp"

#include "graphics/RenderPass.hpp"

#include "graphics/Framebuffer.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/RendererExtensions.hpp"

namespace Engine::Graphics {

auto
RenderPass::update_attachment(Core::u32 index, const Core::Ref<Image>& image)
  -> void
{
  framebuffer->update_attachment(index, image);
}

auto
RenderPass::get_colour_attachment(Core::u32 index) const
  -> const Core::Ref<Image>&
{
  return framebuffer->get_colour_attachment(index);
}

auto
RenderPass::get_depth_attachment() const -> const Core::Ref<Image>&
{
  return framebuffer->get_depth_attachment();
}

auto
RenderPass::bind(CommandBuffer& command_buffer) -> void
{
  RendererExtensions::begin_renderpass(command_buffer, *framebuffer);

  vkCmdBindPipeline(command_buffer.get_command_buffer(),
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline->get_pipeline());
}

auto
RenderPass::unbind(CommandBuffer& command_buffer) -> void
{
  RendererExtensions::end_renderpass(command_buffer);
}

auto
RenderPass::generate_and_update_descriptor_write_sets(Renderer& renderer,
                                                      Material& material)
  -> VkDescriptorSet
{
  return renderer.generate_and_update_descriptor_write_sets(material);
}

} // namespace Engine::Graphic
