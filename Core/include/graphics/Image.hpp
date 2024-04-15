#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace Engine::Graphics {

class Image
{
public:
  VkImage image{ nullptr };
  VmaAllocation allocation{ nullptr };
  VkImageView view{ nullptr };
  VkSampler sampler{ nullptr };

  VkDescriptorImageInfo descriptor_info{};

  auto get_descriptor_info() const -> const VkDescriptorImageInfo&;
};

} // namespace Engine::Graphics
