#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Engine::Graphics {

class Image
{
public:
  VkImage image;
  VmaAllocation allocation;
  VkImageView view;
  VkSampler sampler;

  auto descriptor_info() -> VkDescriptorImageInfo;
};

} // namespace Engine::Graphics
