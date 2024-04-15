#include "pch/CorePCH.hpp"

#include "graphics/RendererExtensions.hpp"

#include "graphics/CommandBuffer.hpp"
#include "graphics/Framebuffer.hpp"

#include <vulkan/vulkan.h>

namespace Engine::Graphics::RendererExtensions {

auto
begin_renderpass(const CommandBuffer& command_buffer,
                 const Framebuffer& framebuffer) -> void
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

  /*
    if (!framebuffer.get_properties().flip_viewport) {
      viewport.y = 0.0F;
      viewport.height = static_cast<float>(framebuffer.get_height());
    }
  */
  vkCmdSetViewport(command_buffer.get_command_buffer(), 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.extent = framebuffer.get_extent();
  vkCmdSetScissor(command_buffer.get_command_buffer(), 0, 1, &scissor);
}

auto end_renderpass(const CommandBuffer& command_buffer) -> void
{
  vkCmdEndRenderPass(command_buffer.get_command_buffer());
}

auto
explicitly_clear_framebuffer(const CommandBuffer& command_buffer,
                             const Framebuffer& framebuffer) -> void
{
  const auto& clear_values = framebuffer.get_clear_values();

  std::vector<VkClearAttachment> clear_attachments;
  clear_attachments.reserve(clear_values.size());

  std::vector<VkClearRect> clear_rects;
  clear_rects.reserve(clear_values.size());

  for (Core::u32 i = 0; i < clear_values.size(); ++i) {
    clear_attachments.emplace_back(
      VK_IMAGE_ASPECT_COLOR_BIT, i, clear_values[i]);
    clear_rects.emplace_back(
      VkRect2D{
        .offset = { 0, 0 },
        .extent = framebuffer.get_extent(),
      },
      0,
      1);
  }

  vkCmdClearAttachments(command_buffer.get_command_buffer(),
                        static_cast<Core::u32>(clear_attachments.size()),
                        clear_attachments.data(),
                        static_cast<Core::u32>(clear_rects.size()),
                        clear_rects.data());
}

} // namespace Engine::Graphics
