#pragma once

#include <vulkan/vulkan.h>

#include "graphics/Forward.hpp"

#include <string>

namespace Engine::Graphics {

class IFramebuffer
{
public:
  virtual ~IFramebuffer() = default;

  // Accessors
  virtual auto get_renderpass() -> VkRenderPass = 0;
  virtual auto get_framebuffer() -> VkFramebuffer = 0;
  virtual auto get_renderpass() const -> VkRenderPass = 0;
  virtual auto get_framebuffer() const -> VkFramebuffer = 0;
  virtual auto get_extent() -> VkExtent2D = 0;
  virtual auto get_extent() const -> VkExtent2D = 0;
  virtual auto get_name() const -> const std::string& = 0;
  virtual auto get_clear_values() const -> const std::vector<VkClearValue>& = 0;
  virtual auto has_depth_attachment() const -> bool = 0;
  virtual auto construct_blend_states() const
    -> std::vector<VkPipelineColorBlendAttachmentState> = 0;

  virtual auto get_colour_attachment(Core::u32) const
    -> const Core::Ref<Image>& = 0;
  virtual auto get_depth_attachment() const -> const Core::Ref<Image>& = 0;

  // Operations
  virtual auto on_resize(const Core::Extent&) -> void = 0;
  virtual auto invalidate() -> void = 0;
  virtual auto release() -> void = 0;
};

} // namespace Engine::Graphics
