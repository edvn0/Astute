#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace Engine::Graphics {

void
create_image(uint32_t width,
             uint32_t height,
             uint32_t mip_levels,
             VkSampleCountFlagBits sample_count,
             VkFormat format,
             VkImageTiling tiling,
             VkImageUsageFlags usage,
             VkImage& image,
             VmaAllocation& allocation,
             VmaAllocationInfo& allocation_info);

void
transition_image_layout(VkImage image,
                        VkFormat format,
                        VkImageLayout old_layout,
                        VkImageLayout new_layout,
                        uint32_t mip_levels);

void
copy_buffer_to_image(VkBuffer buffer,
                     VkImage image,
                     uint32_t width,
                     uint32_t height);

class Image
{
public:
  VkImage image{ nullptr };
  VmaAllocation allocation{ nullptr };
  VmaAllocationInfo allocation_info{};
  VkImageView view{ nullptr };
  VkSampler sampler{ nullptr };

  VkDescriptorImageInfo descriptor_info{};

  auto destroy() -> void;
  auto get_descriptor_info() const -> const VkDescriptorImageInfo&;
};

} // namespace Engine::Graphics
