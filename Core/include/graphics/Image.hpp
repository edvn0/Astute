#pragma once

#include "core/DataBuffer.hpp"

#include "graphics/CommandBuffer.hpp"

#include <vulkan/vulkan.h>

#include <string_view>
#include <unordered_map>

namespace Engine::Graphics {

class Image;

auto to_string(VkFormat) -> std::string_view;
auto to_string(VkImageLayout) -> std::string_view;
auto to_string(VkSampleCountFlagBits) -> std::string_view;

struct ImageConfiguration
{
  Core::u32 width{
    1024,
  };
  Core::u32 height{
    768,
  };
  Core::u32 mip_levels{
    1,
  };
  Core::u32 layers{
    1,
  };
  VkSampleCountFlagBits sample_count{
    VK_SAMPLE_COUNT_1_BIT,
  };

  const VkFormat format{
    VK_FORMAT_R8G8B8A8_UNORM,
  };
  const VkImageTiling tiling{
    VK_IMAGE_TILING_OPTIMAL,
  };

  /// \brief Layout of the image
  const VkImageLayout layout{
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };

  const VkImageUsageFlags usage{
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
  };
  const std::string_view additional_name_data{ "" };

  const VkFilter mag_filter{
    VK_FILTER_LINEAR,
  };
  const VkFilter min_filter{
    VK_FILTER_LINEAR,
  };
  const VkSamplerAddressMode address_mode_u{
    VK_SAMPLER_ADDRESS_MODE_REPEAT,
  };
  const VkSamplerAddressMode address_mode_v{
    VK_SAMPLER_ADDRESS_MODE_REPEAT,
  };
  const VkSamplerAddressMode address_mode_w{
    VK_SAMPLER_ADDRESS_MODE_REPEAT,
  };
  const VkBorderColor border_colour{
    VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
  };

  /// \brief This flag runs transition_image_layout from undefined to layout at
  /// initialisation
  const bool transition_directly{
    false,
  };
};

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

auto
copy_buffer_to_image(VkBuffer buffer,
                     VkImage image,
                     Core::u32 width,
                     Core::u32 height) -> void;

auto
copy_buffer_to_image(const Core::DataBuffer&, Image&) -> void;

auto
create_view(VkImage&,
            VkFormat,
            VkImageAspectFlags,
            Core::u32 mips = 1,
            Core::u32 layer = 1) -> VkImageView;

auto create_sampler(VkFilter, VkSamplerAddressMode, VkBorderColor) -> VkSampler;
auto create_sampler(VkFilter,
                    VkFilter,
                    VkSamplerAddressMode,
                    VkSamplerAddressMode,
                    VkSamplerAddressMode,
                    VkBorderColor) -> VkSampler;

struct ImageImpl;
class Image
{
public:
  VkImage image{ nullptr };
  Core::Scope<ImageImpl> alloc_impl;
  VkImageView view{ nullptr };
  std::unordered_map<Core::u32, VkImageView> layer_image_views{};
  VkSampler sampler{ nullptr };
  VkImageAspectFlags aspect_mask;
  VkDescriptorImageInfo descriptor_info{};
  ImageConfiguration configuration;

  std::optional<Core::usize> hash_value{ std::nullopt };

  bool destroyed{ false };

  ~Image();
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
  auto generate_mips() -> void;

  auto get_mip_levels() const { return configuration.mip_levels; }
  auto get_sample_count() const { return configuration.sample_count; }
  auto get_format() const { return configuration.format; }
  auto get_tiling() const { return configuration.tiling; }
  auto get_layout() const { return configuration.layout; }
  auto get_usage() const { return configuration.usage; }
  auto get_layer_count() const { return configuration.layers; }

  auto hash() -> Core::usize;

  struct Configuration
  {
    const std::string path;
    const VkSampleCountFlagBits sample_count{ VK_SAMPLE_COUNT_1_BIT };
  };
  static auto load_from_file(const Configuration&) -> Core::Ref<Image>;
  static auto load_from_memory(Core::u32,
                               Core::u32,
                               const Core::DataBuffer&,
                               const Configuration&) -> Core::Ref<Image>;
  static auto resolve_msaa(const Image&, const CommandBuffer* = nullptr)
    -> Core::Scope<Image>;
  static auto reference_resolve_msaa(const Image&,
                                     const CommandBuffer* = nullptr)
    -> Core::Ref<Image>;

  static auto construct(const ImageConfiguration&) -> Core::Ref<Image>;

private:
  auto invalidate_hash() -> void;
};

} // namespace Engine::Graphics
