#include "pch/CorePCH.hpp"

#include "graphics/RendererExtensions.hpp"

#include "graphics/CommandBuffer.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/Image.hpp"

#include <array>

#include <vulkan/vulkan.h>

namespace Engine::Graphics::RendererExtensions {

auto
begin_renderpass(const CommandBuffer& command_buffer,
                 const IFramebuffer& framebuffer,
                 const bool flip) -> void
{
  VkRenderPassBeginInfo render_pass_begin_info = {};
  render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_begin_info.renderPass = framebuffer.get_renderpass();
  render_pass_begin_info.framebuffer = framebuffer.get_framebuffer();
  render_pass_begin_info.renderArea.extent = framebuffer.get_extent();
  const auto& clear_values = framebuffer.get_clear_values();
  render_pass_begin_info.clearValueCount =
    static_cast<Core::u32>(clear_values.size());
  render_pass_begin_info.pClearValues = clear_values.data();
  vkCmdBeginRenderPass(command_buffer.get_command_buffer(),
                       &render_pass_begin_info,
                       VK_SUBPASS_CONTENTS_INLINE);

  // Scissors and viewport
  auto&& [width, height] = framebuffer.get_extent();

  VkViewport viewport = {};
  viewport.x = 0.0F;
  viewport.y = static_cast<float>(height);
  viewport.width = static_cast<float>(width);
  viewport.height = -static_cast<float>(height);
  viewport.minDepth = 1.0F;
  viewport.maxDepth = 0.0F;

  if (!flip) {
    viewport.y = 0.0F;
    viewport.height = static_cast<float>(height);
  }

  vkCmdSetViewport(command_buffer.get_command_buffer(), 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.extent = framebuffer.get_extent();
  vkCmdSetScissor(command_buffer.get_command_buffer(), 0, 1, &scissor);
}

auto
end_renderpass(const CommandBuffer& command_buffer) -> void
{
  vkCmdEndRenderPass(command_buffer.get_command_buffer());
}

auto
explicitly_clear_framebuffer(const CommandBuffer& command_buffer,
                             const IFramebuffer& framebuffer,
                             const bool clear_depth) -> void
{
  const std::vector<VkClearValue>& fb_clear_values =
    framebuffer.get_clear_values();

  const auto color_attachment_count = framebuffer.get_colour_attachment_count();
  const auto total_attachment_count =
    color_attachment_count +
    (framebuffer.has_depth_attachment() && clear_depth ? 1 : 0);

  const VkExtent2D extent_2_d = framebuffer.get_extent();
  std::vector<VkClearAttachment> attachments(total_attachment_count);
  std::vector<VkClearRect> clear_rects(total_attachment_count);
  for (Core::u32 i = 0; i < color_attachment_count; i++) {
    attachments[i].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    attachments[i].colorAttachment = i;
    attachments[i].clearValue = fb_clear_values[i];

    clear_rects[i].rect.offset = {
      0,
      0,
    };
    clear_rects[i].rect.extent = extent_2_d;
    clear_rects[i].baseArrayLayer = 0;
    clear_rects[i].layerCount = 1;
  }

  if (framebuffer.has_depth_attachment() && clear_depth) {
    auto aspect_bits = framebuffer.get_depth_attachment()->get_aspect_flags();
    attachments[color_attachment_count].aspectMask = aspect_bits;
    attachments[color_attachment_count].clearValue =
      fb_clear_values[color_attachment_count];
    clear_rects[color_attachment_count].rect.offset = { 0, 0 };
    clear_rects[color_attachment_count].rect.extent = extent_2_d;
    clear_rects[color_attachment_count].baseArrayLayer = 0;
    clear_rects[color_attachment_count].layerCount = 1;
  }

  vkCmdClearAttachments(command_buffer.get_command_buffer(),
                        total_attachment_count,
                        attachments.data(),
                        total_attachment_count,
                        clear_rects.data());
}

auto
bind_vertex_buffer(const CommandBuffer& command,
                   const VertexBuffer& buffer,
                   BufferBinding binding,
                   BufferOffset offset) -> void
{
  std::array<VkDeviceSize, 1> offsets{
    offset,
  };
  auto cmd_buffer = command.get_command_buffer();

  std::array vk_buffers{
    buffer.get_buffer(),
  };

  vkCmdBindVertexBuffers(cmd_buffer,
                         binding,
                         static_cast<Core::u32>(vk_buffers.size()),
                         vk_buffers.data(),
                         offsets.data());
}

auto
bind_index_buffer(const CommandBuffer& command,
                  const IndexBuffer& buffer,
                  BufferBinding,
                  BufferOffset offset) -> void
{
  vkCmdBindIndexBuffer(command.get_command_buffer(),
                       buffer.get_buffer(),
                       offset,
                       VK_INDEX_TYPE_UINT32);
}

} // namespace Engine::Graphics
