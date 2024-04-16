#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace Engine::Graphics {

void
create_image(Core::u32 width,
             Core::u32 height,
             Core::u32 mip_levels,
             VkSampleCountFlagBits sample_count,
             VkFormat format,
             VkImageTiling tiling,
             VkImageUsageFlags usage,
             VkImage& image,
             VmaAllocation& allocation,
             VmaAllocationInfo& allocation_info);

auto
transition_image_layout(
  VkImage image,
  VkImageLayout old_layout,
  VkImageLayout new_layout,
  VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT,
  Core::u32 mip_levels = 1) -> void;

void
copy_buffer_to_image(VkBuffer buffer,
                     VkImage image,
                     Core::u32 width,
                     Core::u32 height);

class Image
{
public:
  VkImage image{ nullptr };
  VmaAllocation allocation{ nullptr };
  VmaAllocationInfo allocation_info{};
  VkImageView view{ nullptr };
  VkSampler sampler{ nullptr };
  VkImageAspectFlags aspect_mask{ VK_IMAGE_ASPECT_COLOR_BIT };

  VkDescriptorImageInfo descriptor_info{};

  auto get_aspect_flags() const -> VkImageAspectFlags { return aspect_mask; }
  auto hash() const -> Core::usize;
  auto destroy() -> void;
  auto get_descriptor_info() const -> const VkDescriptorImageInfo&;
};

} // namespace Engine::Graphics
