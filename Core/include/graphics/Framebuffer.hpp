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
    const std::unordered_map<Core::u32, Core::Ref<Image>> dependent_images{};
    const bool clear_colour_on_load{ true };
    const bool immediate_construction{ true };
    const Core::u32 depth_clear_value{ 0 };
    const std::string name;
  };
  explicit Framebuffer(const Configuration&);

  ~Framebuffer();

  auto get_colour_attachment(Core::u32 index) const -> const Core::Ref<Image>&
  {
    return colour_attachments.at(index);
  }
  auto get_resolved_colour_attachment(Core::u32 index) const
    -> const Core::Ref<Image>&
  {
    return resolved_attachments.at(index);
  }
  auto get_colour_attachment_count() const -> Core::u32
  {
    return static_cast<Core::u32>(colour_attachments.size());
  }
  auto has_depth_attachment() const -> bool
  {
    return depth_attachment != nullptr;
  }
  auto get_depth_attachment(bool resolved = false) const
    -> const Core::Ref<Image>&
  {
    if (resolved_depth_attachment && resolved) {
      return resolved_depth_attachment;
    }
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
  auto get_name() const -> const std::string& { return name; }

  auto add_resolve_for_colour(Core::u32) -> void;
  auto add_resolve_for_depth() -> void;
  auto create_framebuffer_fully() -> void;

private:
  Core::Extent size;
  const std::vector<VkFormat> colour_attachment_formats;
  VkFormat depth_attachment_format;
  const VkSampleCountFlagBits sample_count{ VK_SAMPLE_COUNT_1_BIT };
  const bool resizable;
  std::unordered_map<Core::u32, Core::Ref<Image>> dependent_images{};
  const bool clear_colour_on_load{ true };
  const Core::u32 depth_clear_value{ 0 };
  const std::string name;

  Core::i32 depth_attachment_index{ -1 };

  auto search_dependents_for_depth_format() -> const Image*;

  std::vector<VkClearValue> clear_values;
  std::vector<Core::Ref<Image>> colour_attachments;
  Core::Ref<Image> depth_attachment;

  std::vector<Core::Ref<Image>> resolved_attachments;
  std::vector<std::pair<VkAttachmentDescription2, VkAttachmentReference2>>
    resolved_render_pass_attachments;

  Core::Ref<Image> resolved_depth_attachment;
  std::optional<VkAttachmentDescription2> resolved_depth_attachment_desc{
    std::nullopt
  };

  VkFramebuffer framebuffer{ nullptr };
  VkRenderPass renderpass{ nullptr };

  auto destroy() -> void;
  auto create_colour_attachments() -> void;
  auto create_depth_attachment() -> void;
  auto create_renderpass() -> void;
  void attach_depth_attachments(
    std::vector<VkAttachmentDescription2>& attachments,
    VkAttachmentReference2& depth_attachment_ref);
  void attach_colour_attachments(
    std::vector<VkAttachmentDescription2>& attachments,
    std::vector<VkAttachmentReference2>& color_attachment_refs);
  auto create_framebuffer() -> void;
};

} // namespace Engine::Graphics
