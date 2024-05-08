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
RenderPass::get_depth_attachment(const bool resolved) const
  -> const Core::Ref<Image>&
{
  return get_framebuffer()->get_depth_attachment(resolved);
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
RenderPass::blit_to(const CommandBuffer& command_buffer,
                    const Framebuffer& destination,
                    BlitProperties properties) -> void
{
  if (!properties.colour_attachment_index.has_value() &&
      !properties.depth_attachment) {
    return;
  }

  VkImage src_image;
  VkImageLayout src_layout;
  VkImage dst_image;
  VkImageLayout dst_layout;
  VkImageAspectFlags aspect_flags;
  VkExtent3D extent;
  if (properties.depth_attachment) {
    src_image = get_framebuffer()->get_depth_attachment()->image;
    src_layout = get_framebuffer()->get_depth_attachment()->layout;
    dst_image = destination.get_depth_attachment()->image;
    dst_layout = destination.get_depth_attachment()->layout;

    extent = get_framebuffer()->get_depth_attachment()->extent;

    // Lets assume they share aspect mask.
    aspect_flags = get_framebuffer()->get_depth_attachment()->aspect_mask;
  } else {
    auto& val = properties.colour_attachment_index;
    if (!properties.colour_attachment_index.has_value())
      return;

    src_image = get_framebuffer()->get_colour_attachment(val.value())->image;
    src_layout = get_framebuffer()->get_colour_attachment(val.value())->layout;
    dst_image = destination.get_colour_attachment(val.value())->image;
    dst_layout = destination.get_colour_attachment(val.value())->layout;

    extent = destination.get_colour_attachment(val.value())->extent;
  }

  std::array<VkImageCopy, 1> regions{
    VkImageCopy{
      .srcSubresource =
        VkImageSubresourceLayers{
          .aspectMask = aspect_flags,
          .mipLevel = 0,
          .baseArrayLayer = 0,
          .layerCount = 1,
        },
      .srcOffset = { 0, 0, 0 },
      .dstSubresource =
        VkImageSubresourceLayers{
          .aspectMask = aspect_flags,
          .mipLevel = 0,
          .baseArrayLayer = 0,
          .layerCount = 1,
        },
      .dstOffset = { 0, 0, 0 },
      .extent = extent,
    },
  };

  transition_image_layout(
    src_image, src_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, aspect_flags);
  transition_image_layout(
    dst_image, dst_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, aspect_flags);

  vkCmdCopyImage(command_buffer.get_command_buffer(),
                 src_image,
                 src_layout,
                 dst_image,
                 dst_layout,
                 1,
                 regions.data());

  transition_image_layout(
    src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, src_layout, aspect_flags);
  transition_image_layout(
    dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_layout, aspect_flags);
}

} // namespace Engine::Graphic
