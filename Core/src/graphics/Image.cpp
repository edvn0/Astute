#include "pch/CorePCH.hpp"

#include "core/Types.hpp"

#include "graphics/Allocator.hpp"
#include "graphics/CommandBuffer.hpp"
#include "graphics/Image.hpp"
#include "graphics/Renderer.hpp"

#include "core/DataBuffer.hpp"

#include "core/Verify.hpp"

#include <stb_image.h>

template<>
struct std::hash<VkImage>
{
  auto operator()(VkImage const& image) const -> Engine::Core::usize
  {
    return std::hash<Engine::Core::usize>{}(
      std::bit_cast<Engine::Core::usize>(image));
  }
};

template<>
struct std::formatter<VkFormat> : public std::formatter<std::string_view>
{
  auto format(const VkFormat& type, std::format_context& ctx) const
  {
    return std::format_to(ctx.out(), "{}", Engine::Graphics::to_string(type));
  }
};
template<>
struct std::formatter<VkImageLayout> : public std::formatter<std::string_view>
{
  auto format(const VkImageLayout& type, std::format_context& ctx) const
  {
    return std::format_to(ctx.out(), "{}", Engine::Graphics::to_string(type));
  }
};

namespace Engine::Graphics {

void
create_image(const ImageConfiguration& config,
             VkImage& image,
             VmaAllocation& allocation,
             VmaAllocationInfo& allocation_info)
{
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = config.width;
  imageInfo.extent.height = config.height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = config.mip_levels;
  imageInfo.arrayLayers = config.layers;
  imageInfo.format = config.format;
  imageInfo.tiling = config.tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = config.usage;
  imageInfo.samples = config.sample_count;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  Allocator allocator{
    std::format("Format-{}-SampleCount-{}-AdditionalData-{}",
                to_string(config.format),
                to_string(config.sample_count),
                config.additional_name_data),
  };

  AllocationProperties alloc_props{};
  alloc_props.usage = Usage::AUTO_PREFER_DEVICE;
  if (config.sample_count != VK_SAMPLE_COUNT_1_BIT) {
    alloc_props.flags = RequiredFlags::LAZILY_ALLOCATED_BIT;
  }

  alloc_props.priority = 1.0F;

  allocation =
    allocator.allocate_image(image, allocation_info, imageInfo, alloc_props);

  trace("Created image '{}', Vulkan pointer: {}",
        config.additional_name_data,
        (const void*)image);
}

void
transition_image_layout(VkCommandBuffer buffer,
                        VkImage image,
                        VkImageLayout old_layout,
                        VkImageLayout new_layout,
                        VkImageAspectFlags aspect_mask,
                        Core::u32 mip_levels)
{
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = aspect_mask;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = mip_levels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
             new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else {
    error("unsupported layout transition between {} and {}",
          old_layout,
          new_layout);

    throw Core::InvalidOperationException(
      "unsupported layout transition between {} and {}",
      old_layout,
      new_layout);
  }

  vkCmdPipelineBarrier(buffer,
                       sourceStage,
                       destinationStage,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);
}

void
transition_image_layout(VkImage image,
                        VkImageLayout old_layout,
                        VkImageLayout new_layout,
                        VkImageAspectFlags aspect_mask,
                        Core::u32 mip_levels)
{
  Device::the().execute_immediate([&](VkCommandBuffer buffer) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspect_mask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mip_levels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
      barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout ==
                 VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
      barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout ==
                 VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
      barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
      barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

      destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
               new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
               new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

      sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
      error("unsupported layout transition between {} and {}",
            old_layout,
            new_layout);

      throw Core::InvalidOperationException(
        "unsupported layout transition between {} and {}",
        old_layout,
        new_layout);
    }

    vkCmdPipelineBarrier(buffer,
                         sourceStage,
                         destinationStage,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);
  });
}

void
copy_buffer_to_image(VkBuffer buffer,
                     VkImage image,
                     Core::u32 width,
                     Core::u32 height)
{
  Device::the().execute_immediate([&](auto* command_buffer) {
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(command_buffer,
                           buffer,
                           image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &region);
  });
}

auto
create_view(VkImage& image, VkFormat format, VkImageAspectFlags aspect_mask)
  -> VkImageView
{
  VkImageViewCreateInfo view_create_info{};
  view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_create_info.image = image;
  view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_create_info.format = format;
  view_create_info.subresourceRange.aspectMask = aspect_mask;
  view_create_info.subresourceRange.baseMipLevel = 0;
  view_create_info.subresourceRange.levelCount = 1;
  view_create_info.subresourceRange.baseArrayLayer = 0;
  view_create_info.subresourceRange.layerCount = 1;
  view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

  VkImageView view;
  VK_CHECK(vkCreateImageView(
    Device::the().device(), &view_create_info, nullptr, &view));
  return view;
}

auto
create_sampler(VkFilter min_filter,
               VkFilter mag_filter,
               VkSamplerAddressMode u_address_mode,
               VkSamplerAddressMode v_address_mode,
               VkSamplerAddressMode w_address_mode,
               VkBorderColor border_color) -> VkSampler
{
  VkSamplerCreateInfo sampler_info{};
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter = min_filter;
  sampler_info.minFilter = mag_filter;
  sampler_info.addressModeU = u_address_mode;
  sampler_info.addressModeV = v_address_mode;
  sampler_info.addressModeW = w_address_mode;
  sampler_info.anisotropyEnable = VK_TRUE;
  sampler_info.maxAnisotropy = 16;
  sampler_info.borderColor = border_color;
  sampler_info.unnormalizedCoordinates = VK_FALSE;
  sampler_info.compareEnable = VK_TRUE;
  sampler_info.compareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.mipLodBias = 0.0f;
  sampler_info.minLod = 0.0f;
  sampler_info.maxLod = 1.0f;

  VkSampler sampler;
  VK_CHECK(
    vkCreateSampler(Device::the().device(), &sampler_info, nullptr, &sampler));
  return sampler;
}

auto
create_sampler(VkFilter filter, VkSamplerAddressMode mode, VkBorderColor col)
  -> VkSampler
{
  return create_sampler(filter, filter, mode, mode, mode, col);
}

auto
Image::destroy() -> void
{
  if (destroyed)
    return;

  if (image == VK_NULL_HANDLE || view == VK_NULL_HANDLE ||
      sampler == VK_NULL_HANDLE) {
    return;
  }

  vkDestroyImageView(Device::the().device(), view, nullptr);
  vkDestroySampler(Device::the().device(), sampler, nullptr);
  Allocator allocator{ "destroy_image" };
  allocator.deallocate_image(allocation, image);

  destroyed = true;
}

auto
Image::get_descriptor_info() const -> const VkDescriptorImageInfo&
{
  return descriptor_info;
}

auto
Image::hash() const -> Core::usize
{
  return std::hash<VkImage>{}(image);
}

auto
Image::load_from_file(const Configuration& config) -> Core::Ref<Image>
{
  Core::Ref<Image> image = Core::make_ref<Image>();

  std::filesystem::path whole_path{ config.path };

  if (!std::filesystem::exists(whole_path)) {
    error("Could not find image at '{}'", whole_path.string());
    return Renderer::get_white_texture();
  }

  Core::i32 width{};
  Core::i32 height{};
  Core::i32 channels{};
  auto* pixel_data = stbi_load(
    whole_path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);

  Core::DataBuffer data_buffer{ width * height * 4 };
  data_buffer.write(
    std::span{ pixel_data, static_cast<Core::usize>(width * height * 4) });
  stbi_image_free(pixel_data);

  return load_from_memory(static_cast<Core::u32>(width),
                          static_cast<Core::u32>(height),
                          data_buffer,
                          config);
}

auto
Image::load_from_memory(Core::u32 width,
                        Core::u32 height,
                        const Core::DataBuffer& data_buffer,
                        const Configuration& config) -> Core::Ref<Image>
{

  Core::Ref<Image> image = Core::make_ref<Image>(ImageConfiguration{
    .width = width,
    .height = height,
    .sample_count = config.sample_count,
    .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
             VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
  });

  Allocator allocator{ "Image" };
  VkBufferCreateInfo buffer_create_info{};
  buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_create_info.size = data_buffer.size();
  buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  VkBuffer staging_buffer{};
  VmaAllocationInfo staging_allocation_info{};
  const auto allocation = allocator.allocate_buffer(
    staging_buffer,
    staging_allocation_info,
    buffer_create_info,
    {
      .usage = Usage::AUTO_PREFER_DEVICE,
      .creation = Creation::HOST_ACCESS_RANDOM_BIT | Creation::MAPPED_BIT,
    });
  const std::span allocation_span{
    static_cast<Core::u8*>(staging_allocation_info.pMappedData),
    staging_allocation_info.size,
  };
  data_buffer.read(allocation_span);

  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = 0; // Initially unsignaled
  VkFence fence;
  vkCreateFence(Device::the().device(), &fence_info, nullptr, &fence);

  Device::the().execute_immediate(
    [width, height, &staging_buffer, &image](auto* cmd_buffer) {
      transition_image_layout(cmd_buffer,
                              image->image,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
      VkBufferImageCopy region{};
      region.bufferOffset = 0;
      region.bufferRowLength = 0;
      region.bufferImageHeight = 0;
      region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.mipLevel = 0;
      region.imageSubresource.baseArrayLayer = 0;
      region.imageSubresource.layerCount = 1;
      region.imageOffset = {
        0,
        0,
        0,
      };
      region.imageExtent = {
        width,
        height,
        1,
      };

      vkCmdCopyBufferToImage(cmd_buffer,
                             staging_buffer,
                             image->image,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             1,
                             &region);

      transition_image_layout(cmd_buffer,
                              image->image,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    },
    fence);

  // Wait for the fence to signal that command buffer execution is complete
  vkWaitForFences(Device::the().device(), 1, &fence, VK_TRUE, UINT64_MAX);
  vkDestroyFence(Device::the().device(), fence, nullptr);
  allocator.deallocate_buffer(allocation, staging_buffer);

  return image;
}

auto
Image::resolve_msaa(const Image& source, const CommandBuffer* command_buffer)
  -> Core::Scope<Image>
{
  return nullptr;
}

auto
Image::reference_resolve_msaa(const Image& source,
                              const CommandBuffer* command_buffer)
  -> Core::Ref<Image>
{
  return nullptr;
}

static constexpr auto depth_formats = std::array{
  VK_FORMAT_D32_SFLOAT,         VK_FORMAT_D16_UNORM,
  VK_FORMAT_D16_UNORM_S8_UINT,  VK_FORMAT_D24_UNORM_S8_UINT,
  VK_FORMAT_D32_SFLOAT_S8_UINT,
};

static constexpr auto is_depth_format = [](VkFormat format) {
  for (const auto& fmt : depth_formats) {
    if (format == fmt)
      return true;
  }
  return false;
};
static constexpr auto to_aspect_mask = [](VkFormat fmt) {
  if (is_depth_format(fmt)) {
    VkImageAspectFlags depth_flag = VK_IMAGE_ASPECT_DEPTH_BIT;

    if (fmt == VK_FORMAT_D24_UNORM_S8_UINT ||
        fmt == VK_FORMAT_D16_UNORM_S8_UINT ||
        fmt == VK_FORMAT_D32_SFLOAT_S8_UINT) {
      depth_flag |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    return depth_flag;
  }

  return static_cast<VkImageAspectFlags>(VK_IMAGE_ASPECT_COLOR_BIT);
};

Image::Image(const ImageConfiguration& conf)
  : aspect_mask(to_aspect_mask(conf.format))
  , configuration(conf)
{
  configuration.additional_name_data = "Colour Attachment MSAA";
  invalidate();
}

auto
Image::create_specific_layer_image_views(
  const std::span<const Core::u32> indices) -> void
{
}

auto
Image::invalidate() -> void
{
  destroy();

  create_image(configuration, image, allocation, allocation_info);
  view = create_view(image, configuration.format, aspect_mask);
  sampler = create_sampler(configuration.min_filter,
                           configuration.mag_filter,
                           configuration.address_mode_u,
                           configuration.address_mode_v,
                           configuration.address_mode_w,
                           configuration.border_colour);

  descriptor_info.imageLayout = configuration.layout;
  descriptor_info.imageView = view;
  descriptor_info.sampler = sampler;

  destroyed = false;
}

} // namespace Engine::Graphic
