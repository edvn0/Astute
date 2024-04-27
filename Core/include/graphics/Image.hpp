#pragma once

#include "core/DataBuffer.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <string_view>

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
             VmaAllocationInfo& allocation_info,
             std::string_view additional_name_data = "");

auto
transition_image_layout(
  VkCommandBuffer,
  VkImage image,
  VkImageLayout old_layout,
  VkImageLayout new_layout,
  VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT,
  Core::u32 mip_levels = 1) -> void;

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

auto
create_view(VkImage&, VkFormat, VkImageAspectFlags) -> VkImageView;

auto create_sampler(VkFilter, VkSamplerAddressMode, VkBorderColor) -> VkSampler;

class Image
{
public:
  VkImage image{ nullptr };
  VmaAllocation allocation{ nullptr };
  VmaAllocationInfo allocation_info{};
  VkImageView view{ nullptr };
  VkSampler sampler{ nullptr };
  VkImageAspectFlags aspect_mask{ VK_IMAGE_ASPECT_COLOR_BIT };
  VkFormat format{ VK_FORMAT_UNDEFINED };
  VkSampleCountFlagBits sample_count{ VK_SAMPLE_COUNT_1_BIT };
  VkImageLayout layout{ VK_IMAGE_LAYOUT_UNDEFINED };
  VkDescriptorImageInfo descriptor_info{};
  VkExtent3D extent{};

  bool destroyed{ false };

  ~Image()
  {
    if (!destroyed) {
      destroy();
      destroyed = true;
    }
  }
  Image() = default;
  Image(const Image&) = delete;
  auto operator=(const Image&) -> Image& = delete;
  Image(Image&&) = delete;
  auto operator=(Image&&) -> Image& = delete;

  auto get_aspect_flags() const -> VkImageAspectFlags { return aspect_mask; }
  auto hash() const -> Core::usize;
  auto destroy() -> void;
  auto get_descriptor_info() const -> const VkDescriptorImageInfo&;

  struct Configuration
  {
    const std::string path;
    const VkSampleCountFlagBits sample_count{ VK_SAMPLE_COUNT_1_BIT };
  };
  static auto load_from_file(const Configuration& = {}) -> Core::Ref<Image>;
  static auto load_from_memory(Core::u32,
                               Core::u32,
                               const Core::DataBuffer&,
                               const Configuration& = {}) -> Core::Ref<Image>;
};

} // namespace Engine::Graphics
