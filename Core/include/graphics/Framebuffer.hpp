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
    const std::vector<VkFormat> colour_attachment_formats{};
    const VkFormat depth_attachment_format{ VK_FORMAT_UNDEFINED };
    const VkSampleCountFlagBits sample_count{ VK_SAMPLE_COUNT_1_BIT };
    const bool resizable{ true };
    const std::vector<Core::Ref<Image>> dependent_attachments{};
    const std::string name;
  };
  explicit Framebuffer(const Configuration&);

  ~Framebuffer();

  auto get_colour_attachment(Core::u32 index) const -> const Core::Ref<Image>&
  {
    if (is_msaa()) {
      auto actual = 2 * index + 1;
      return colour_attachments[actual];
    }
    return colour_attachments[index];
  }
  auto get_colour_attachment_count() const -> Core::u32
  {
    return static_cast<Core::u32>(colour_attachments.size());
  }
  auto has_depth_attachment() const -> bool
  {
    return depth_attachment != nullptr;
  }
  auto get_depth_attachment() const -> const Core::Ref<Image>&
  {
    return depth_attachment;
  }
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
  auto is_msaa() const -> bool { return sample_count != VK_SAMPLE_COUNT_1_BIT; }
  auto construct_blend_states() const
    -> std::vector<VkPipelineColorBlendAttachmentState>;

  auto on_resize(const Core::Extent&) -> void;
  auto update_attachment(Core::u32 index, const Core::Ref<Image>& image) -> void
  {
    dependent_attachments[index] = image;
  }

  auto get_name() const -> const std::string& { return name; }

private:
  Core::Extent size;
  const std::vector<VkFormat> colour_attachment_formats;
  VkFormat depth_attachment_format;
  const VkSampleCountFlagBits sample_count{ VK_SAMPLE_COUNT_1_BIT };
  const bool resizable;
  // Not const because we might need to update the pointers on resize
  std::vector<Core::Ref<Image>> dependent_attachments{};
  const std::string name;

  auto search_dependents_for_depth_format() -> const Image*;

  std::vector<VkClearValue> clear_values;
  std::vector<Core::Ref<Image>> colour_attachments;
  Core::Ref<Image> depth_attachment;

  VkFramebuffer framebuffer{ VK_NULL_HANDLE };
  VkRenderPass renderpass{ VK_NULL_HANDLE };

  auto destroy() -> void;
  auto create_colour_attachments() -> void;
  auto create_depth_attachment() -> void;
  auto create_renderpass() -> void;
  auto create_framebuffer() -> void;
};

} // namespace Engine::Graphics
