#pragma once

#include "core/Types.hpp"
#include "graphics/Forward.hpp"

#include <vulkan/vulkan.h>

namespace Engine::Graphics {

class Framebuffer
{
public:
  ~Framebuffer() = default;

  auto get_colour_attachment(Core::u32) -> const Image* { return nullptr; }
  auto get_depth_attachment() -> const Image* { return nullptr; }
  auto get_renderpass() -> VkRenderPass { return VK_NULL_HANDLE; };
  auto get_framebuffer() -> VkFramebuffer { return VK_NULL_HANDLE; };
  auto get_extent() -> VkExtent2D { return { 0, 0 }; };
  auto get_clear_values() const -> const std::vector<VkClearValue>&
  {
    return clear_values;
  };

private:
  std::vector<VkClearValue> clear_values;
};

} // namespace Engine::Graphics
