#pragma once

#include "core/Types.hpp"
#include "graphics/Forward.hpp"

#include <vulkan/vulkan.h>

namespace Engine::Graphics {

class Framebuffer
{
public:
  struct Configuration
  {
    const Core::Extent size;
  };
  explicit Framebuffer(Configuration);

  ~Framebuffer();

  auto get_colour_attachment(Core::u32) -> const Image* { return nullptr; }
  auto get_depth_attachment() -> const Image* { return nullptr; }
  auto get_renderpass() -> VkRenderPass { return renderpass; }
  auto get_renderpass() const -> VkRenderPass { return renderpass; }
  auto get_framebuffer() -> VkFramebuffer { return framebuffer; }
  auto get_framebuffer() const -> VkFramebuffer { return framebuffer; }
  auto get_extent() -> VkExtent2D { return { size.width, size.height }; }
  auto get_extent() const -> VkExtent2D { return { size.width, size.height }; }
  auto get_clear_values() const -> const std::vector<VkClearValue>&
  {
    return clear_values;
  }

private:
  Core::Extent size;

  std::vector<VkClearValue> clear_values;
  std::vector<Core::Scope<Image>> colour_attachments;
  Core::Scope<Image> depth_attachment;

  VkFramebuffer framebuffer{ VK_NULL_HANDLE };
  VkRenderPass renderpass{ VK_NULL_HANDLE };

  auto create_colour_attachments() -> void;
  auto create_depth_attachment() -> void;
  auto create_renderpass() -> void;
  auto create_framebuffer() -> void;
};

} // namespace Engine::Graphics
