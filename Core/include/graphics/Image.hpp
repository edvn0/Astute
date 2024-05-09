#pragma once

#include "core/DataBuffer.hpp"

#include "graphics/CommandBuffer.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <string_view>
#include <unordered_map>

namespace Engine::Graphics {

auto to_string(VkFormat) -> std::string_view;
auto to_string(VkImageLayout) -> std::string_view;
auto to_string(VkSampleCountFlagBits) -> std::string_view;

struct ImageConfiguration
{
  Core::u32 width{ 1024 };
  Core::u32 height{ 768 };
  Core::u32 mip_levels{ 1 };
  VkSampleCountFlagBits sample_count{ VK_SAMPLE_COUNT_1_BIT };
  VkFormat format{ VK_FORMAT_R8G8B8A8_UNORM };
  VkImageTiling tiling{ VK_IMAGE_TILING_OPTIMAL };
  VkImageLayout layout{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
  VkImageUsageFlags usage{ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                           VK_IMAGE_USAGE_SAMPLED_BIT |
                           VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                           VK_IMAGE_USAGE_TRANSFER_DST_BIT };
  Core::u32 layers{ 1 };
  std::string_view additional_name_data{ "" };

  VkFilter mag_filter{ VK_FILTER_LINEAR };
  VkFilter min_filter{ VK_FILTER_LINEAR };
  VkSamplerAddressMode address_mode_u{ VK_SAMPLER_ADDRESS_MODE_REPEAT };
  VkSamplerAddressMode address_mode_v{ VK_SAMPLER_ADDRESS_MODE_REPEAT };
  VkSamplerAddressMode address_mode_w{ VK_SAMPLER_ADDRESS_MODE_REPEAT };
  VkBorderColor border_colour{ VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK };
};

void
create_image(const ImageConfiguration&,
             VkImage&,
             VmaAllocation&,
             VmaAllocationInfo&);

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
auto create_sampler(VkFilter,
                    VkFilter,
                    VkSamplerAddressMode,
                    VkSamplerAddressMode,
                    VkSamplerAddressMode,
                    VkBorderColor) -> VkSampler;

class Image
{
public:
  VkImage image{ nullptr };
  VmaAllocation allocation{ nullptr };
  VmaAllocationInfo allocation_info{};
  VkImageView view{ nullptr };
  std::unordered_map<Core::u32, VkImageView> layer_image_views{};
  VkSampler sampler{ nullptr };
  VkImageAspectFlags aspect_mask;
  VkDescriptorImageInfo descriptor_info{};
  ImageConfiguration configuration;

  bool destroyed{ false };

  ~Image()
  {
    if (!destroyed) {
      destroy();
      destroyed = true;
    }
  }
  Image() = default;
  Image(const ImageConfiguration&);

  Image(const Image&) = delete;
  auto operator=(const Image&) -> Image& = delete;
  Image(Image&&) = delete;
  auto operator=(Image&&) -> Image& = delete;

  auto get_aspect_flags() const -> VkImageAspectFlags { return aspect_mask; }
  auto hash() const -> Core::usize;
  auto destroy() -> void;
  auto get_descriptor_info() const -> const VkDescriptorImageInfo&;

  auto get_layer_image_view(Core::u32 index) -> VkImageView
  {
    return layer_image_views.at(index);
  }
  auto create_specific_layer_image_views(
    const std::span<const Core::u32> indices) -> void;
  auto invalidate() -> void;

  auto get_mip_levels() const { return configuration.mip_levels; }
  auto get_sample_count() const { return configuration.sample_count; }
  auto get_format() const { return configuration.format; }
  auto get_tiling() const { return configuration.tiling; }
  auto get_layout() const { return configuration.layout; }
  auto get_usage() const { return configuration.usage; }
  auto get_layer_count() const { return configuration.layers; }

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
  static auto resolve_msaa(const Image&, const CommandBuffer* = nullptr)
    -> Core::Scope<Image>;
  static auto reference_resolve_msaa(const Image&,
                                     const CommandBuffer* = nullptr)
    -> Core::Ref<Image>;
};

} // namespace Engine::Graphics
