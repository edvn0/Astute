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
             const std::string_view additional_name_data)
{
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = mip_levels;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = sample_count;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  Allocator allocator{
    std::format("Format-{}-SampleCount-{}-AdditionalData-{}",
                to_string(format),
                to_string(sample_count),
                additional_name_data),
  };

  AllocationProperties alloc_props{};
  alloc_props.usage = Usage::AUTO_PREFER_DEVICE;
  if (sample_count != VK_SAMPLE_COUNT_1_BIT) {
    alloc_props.flags = RequiredFlags::LAZILY_ALLOCATED_BIT;
  }

  alloc_props.priority = 1.0F;

  allocation =
    allocator.allocate_image(image, allocation_info, imageInfo, alloc_props);

  trace("Created image '{}', Vulkan pointer: {}",
        additional_name_data,
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
create_view(VkImage& image,
            VkFormat format,
            VkImageAspectFlags aspect_mask) -> VkImageView
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
create_sampler(VkFilter filter,
               VkSamplerAddressMode address_mode,
               VkBorderColor border_color) -> VkSampler
{
  VkSamplerCreateInfo sampler_info{};
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter = filter;
  sampler_info.minFilter = filter;
  sampler_info.addressModeU = address_mode;
  sampler_info.addressModeV = address_mode;
  sampler_info.addressModeW = address_mode;
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
  Core::Ref<Image> image = Core::make_ref<Image>();

  create_image(width,
               height,
               1,
               config.sample_count,
               VK_FORMAT_R8G8B8A8_UNORM,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                 VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT,
               image->image,
               image->allocation,
               image->allocation_info,
               config.path);
  image->sample_count = config.sample_count;
  image->extent = { width, height, 1 };
  image->format = VK_FORMAT_R8G8B8A8_UNORM;
  image->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image->aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
  image->view = create_view(
    image->image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
  image->sampler = create_sampler(VK_FILTER_LINEAR,
                                  VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                  VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);

  auto& descriptor_info = image->descriptor_info;
  descriptor_info.imageLayout = image->layout;
  descriptor_info.imageView = image->view;
  descriptor_info.sampler = image->sampler;

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
Image::resolve_msaa(const Image& source,
                    const CommandBuffer* command_buffer) -> Core::Scope<Image>
{
  static constexpr auto is_depth_format = [](VkFormat format) -> bool {
    return format == VK_FORMAT_D32_SFLOAT ||
           format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
           format == VK_FORMAT_D24_UNORM_S8_UINT;
  };

  Core::Scope<Image> image = Core::make_scope<Image>();
  create_image(
    source.extent.width,
    source.extent.height,
    1,
    VK_SAMPLE_COUNT_1_BIT,
    source.format,
    VK_IMAGE_TILING_OPTIMAL,
    is_depth_format(source.format)
      ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
          VK_IMAGE_USAGE_TRANSFER_SRC_BIT
      : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
          VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    image->image,
    image->allocation,
    image->allocation_info,
    "ResolveMSAA");
  image->view = create_view(image->image, source.format, source.aspect_mask);
  image->sampler = create_sampler(VK_FILTER_LINEAR,
                                  VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                  VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);
  image->sample_count = VK_SAMPLE_COUNT_1_BIT;
  image->extent = source.extent;
  image->format = source.format;
  image->layout = source.layout;
  image->aspect_mask = source.aspect_mask;

  auto& descriptor_info = image->descriptor_info;
  descriptor_info.imageLayout = image->layout;
  descriptor_info.imageView = image->view;
  descriptor_info.sampler = image->sampler;

  transition_image_layout(image->image,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          source.aspect_mask);

  transition_image_layout(source.image,
                          source.layout,
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          source.aspect_mask);

  if (command_buffer == nullptr) {
    Device::the().execute_immediate(
      [&source, &image](VkCommandBuffer cmd_buffer) {
        VkImageResolve region{};
        region.srcSubresource.aspectMask = source.aspect_mask;
        region.srcSubresource.mipLevel = 0;
        region.srcSubresource.baseArrayLayer = 0;
        region.srcSubresource.layerCount = 1;
        region.srcOffset = { 0, 0, 0 };
        region.dstSubresource = region.srcSubresource;
        region.dstOffset = { 0, 0, 0 };
        region.extent = source.extent;

        vkCmdResolveImage(cmd_buffer,
                          source.image,
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          image->image,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          1,
                          &region);
      });
  } else {
    VkImageResolve region{};
    region.srcSubresource.aspectMask = source.aspect_mask;
    region.srcSubresource.mipLevel = 0;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount = 1;
    region.srcOffset = { 0, 0, 0 };
    region.dstSubresource = region.srcSubresource;
    region.dstOffset = { 0, 0, 0 };
    region.extent = source.extent;

    vkCmdResolveImage(command_buffer->get_command_buffer(),
                      source.image,
                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      image->image,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      1,
                      &region);
  }

  transition_image_layout(image->image,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          image->layout,
                          source.aspect_mask);

  transition_image_layout(source.image,
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          source.layout,
                          source.aspect_mask);
  return image;
}

auto
Image::reference_resolve_msaa(const Image& source,
                              const CommandBuffer* command_buffer)
  -> Core::Ref<Image>
{
  auto resolved = resolve_msaa(source, command_buffer);
  return Core::Ref<Image>(resolved.release());
}

} // namespace Engine::Graphic
