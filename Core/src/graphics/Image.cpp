#include "pch/CorePCH.hpp"

#include "core/Types.hpp"

#include "graphics/Allocator.hpp"
#include "graphics/CommandBuffer.hpp"
#include "graphics/Image.hpp"
#include "graphics/Renderer.hpp"

#include "core/DataBuffer.hpp"

#include "core/Verify.hpp"

#include <span>
#include <stb_image.h>
#include <stb_image_write.h>
#include <vk_mem_alloc.h>

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
  static auto format(const VkFormat& type, std::format_context& ctx)
  {
    return std::format_to(ctx.out(), "{}", Engine::Graphics::to_string(type));
  }
};
template<>
struct std::formatter<VkImageLayout> : public std::formatter<std::string_view>
{
  static auto format(const VkImageLayout& type, std::format_context& ctx)
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
  if (config.is_transfer) {
    imageInfo.usage |=
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }
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
                        Core::u32 mip_levels,
                        Core::u32 current_mip_base)
{
  VkImageMemoryBarrier image_memory_barrier{};
  image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  image_memory_barrier.oldLayout = old_layout;
  image_memory_barrier.newLayout = new_layout;
  image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  image_memory_barrier.image = image;
  image_memory_barrier.subresourceRange.aspectMask = aspect_mask;
  image_memory_barrier.subresourceRange.baseMipLevel = current_mip_base;
  image_memory_barrier.subresourceRange.levelCount = mip_levels;
  image_memory_barrier.subresourceRange.baseArrayLayer = 0;
  image_memory_barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  switch (old_layout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
      image_memory_barrier.srcAccessMask = 0;
      sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      break;
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
      image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
      sourceStage = VK_PIPELINE_STAGE_HOST_BIT;
      break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
      image_memory_barrier.srcAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      image_memory_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
      sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      break;
    default:
      sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
      break;
  }

  switch (new_layout) {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
      image_memory_barrier.dstAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      break;
    default:
      destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
      break;
  }

  vkCmdPipelineBarrier(buffer,
                       sourceStage,
                       destinationStage,
                       0, // No dependency flags
                       0,
                       nullptr, // No memory barriers
                       0,
                       nullptr, // No buffer memory barriers
                       1,
                       &image_memory_barrier // Image memory barrier
  );
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

void
copy_buffer_to_image(VkCommandBuffer buf,
                     VkBuffer buffer,
                     VkImage image,
                     Core::u32 width,
                     Core::u32 height)
{
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

  vkCmdCopyBufferToImage(
    buf, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

auto
copy_buffer_to_image(const Core::DataBuffer& data, Image& image) -> void
{
  StagingBuffer buffer{ data.span() };
  copy_buffer_to_image(buffer.get_buffer(),
                       image.image,
                       image.configuration.width,
                       image.configuration.height);
}

auto
copy_buffer_to_image(VkCommandBuffer buf,
                     const Core::DataBuffer& data,
                     Image& image) -> void
{
  StagingBuffer buffer{ data.span() };
  copy_buffer_to_image(buf,
                       buffer.get_buffer(),
                       image.image,
                       image.configuration.width,
                       image.configuration.height);
}

auto
create_sampler(VkFilter min_filter,
               VkFilter mag_filter,
               VkSamplerAddressMode u_address_mode,
               VkSamplerAddressMode v_address_mode,
               VkSamplerAddressMode w_address_mode,
               VkBorderColor border_colour,
               Core::u32 mips) -> VkSampler
{
  VkSamplerCreateInfo sampler_info{};
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter = min_filter;
  sampler_info.minFilter = mag_filter;
  sampler_info.addressModeU = u_address_mode;
  sampler_info.addressModeV = v_address_mode;
  sampler_info.addressModeW = w_address_mode;
  sampler_info.anisotropyEnable = VK_TRUE;
  if (mips == 0) {
    sampler_info.anisotropyEnable = VK_FALSE;
  }
  sampler_info.maxAnisotropy = 16;
  sampler_info.borderColor = border_colour;
  sampler_info.unnormalizedCoordinates = VK_FALSE;
  sampler_info.compareEnable = VK_TRUE;
  sampler_info.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.mipLodBias = 0.0F;
  sampler_info.minLod = 0.0F;
  sampler_info.maxLod = static_cast<Core::f32>(mips);

  VkSampler sampler;
  VK_CHECK(
    vkCreateSampler(Device::the().device(), &sampler_info, nullptr, &sampler));
  return sampler;
}

auto
create_sampler(VkFilter filter,
               VkSamplerAddressMode mode,
               VkBorderColor col,
               Core::u32 mips) -> VkSampler
{
  return create_sampler(filter, filter, mode, mode, mode, col, mips);
}

struct ImageImpl
{
  VmaAllocation allocation{};
  VmaAllocationInfo allocation_info{};
};

auto
Image::destroy() -> void
{
  if (destroyed) {
    return;
  }

  if (image == VK_NULL_HANDLE || view == VK_NULL_HANDLE ||
      sampler == VK_NULL_HANDLE) {
    return;
  }

  vkDestroyImageView(Device::the().device(), view, nullptr);
  vkDestroySampler(Device::the().device(), sampler, nullptr);

  for (const auto& [index, layer_view] : layer_image_views) {
    vkDestroyImageView(Device::the().device(), layer_view, nullptr);
  }
  layer_image_views.clear();
  for (const auto& [index, layer_view] : mip_image_views) {
    vkDestroyImageView(Device::the().device(), layer_view, nullptr);
  }
  mip_image_views.clear();

  Allocator allocator{ "destroy_image" };
  allocator.deallocate_image(alloc_impl->allocation, image);
  alloc_impl.reset(new ImageImpl);

  destroyed = true;
}

Image::~Image()
{
  if (!destroyed) {
    destroy();
    destroyed = true;
  }
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
Image::load_from_file_into_staging(const std::string_view path,
                                   Core::u32* out_w,
                                   Core::u32* out_h) -> Core::Ref<StagingBuffer>
{
  std::filesystem::path whole_path{ path };

  if (!std::filesystem::exists(whole_path)) {
    error("Could not find image at '{}'", whole_path.string());
    throw std::runtime_error("Could not find image");
  }

  Core::i32 width{};
  Core::i32 height{};
  Core::i32 channels{};
  auto* pixel_data = stbi_load(
    whole_path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);

  Core::DataBuffer data_buffer{
    width * height * STBI_rgb_alpha,
  };
  data_buffer.write(std::span{
    pixel_data, static_cast<Core::usize>(width * height * STBI_rgb_alpha) });
  trace("Loaded image from file '{}', size: {}",
        whole_path.string(),
        data_buffer.size());
  stbi_image_free(pixel_data);

  if (out_w != nullptr) {
    *out_w = static_cast<Core::u32>(width);
  }
  if (out_h != nullptr) {
    *out_h = static_cast<Core::u32>(height);
  }

  return Core::make_ref<StagingBuffer>(std::move(data_buffer));
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
    .mip_levels =
      config.use_mips ? compute_mips_from_width_height(width, height) : 1,
    .sample_count = config.sample_count,
    .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
             VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    .additional_name_data = std::format("LoadedFromMemory@{}", config.path) });

  Allocator allocator{ "Image" };
  VkBufferCreateInfo buffer_create_info{};
  buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_create_info.size = data_buffer.size();
  buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  VkBuffer staging_buffer{};
  VmaAllocationInfo staging_allocation_info{};
  auto* allocation = allocator.allocate_buffer(
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
  fence_info.flags = 0;
  VkFence fence;
  vkCreateFence(Device::the().device(), &fence_info, nullptr, &fence);

  Device::the().execute_immediate(
    [width, height, &staging_buffer, &image](auto* cmd_buffer) {
      transition_image_layout(cmd_buffer,
                              image->image,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              image->get_aspect_flags(),
                              image->get_mip_levels());
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

      if (image->get_mip_levels() > 1) {
        image->generate_mips(cmd_buffer);
      } else {
        transition_image_layout(cmd_buffer,
                                image->image,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                image->get_layout(),
                                image->get_aspect_flags(),
                                image->get_mip_levels());
      }
    },
    fence);

  // Wait for the fence to signal that command buffer execution is complete
  vkWaitForFences(Device::the().device(), 1, &fence, VK_TRUE, UINT64_MAX);
  vkDestroyFence(Device::the().device(), fence, nullptr);
  allocator.deallocate_buffer(allocation, staging_buffer);

  return image;
}

auto
Image::load_from_memory(const CommandBuffer* buffer,
                        Core::u32 width,
                        Core::u32 height,
                        Core::Ref<Graphics::StagingBuffer> staging_buffer,
                        const Configuration& config) -> Core::Ref<Image>
{
  Core::Ref<Image> image = Core::make_ref<Image>(ImageConfiguration{
    .width = width,
    .height = height,
    .mip_levels =
      config.use_mips ? compute_mips_from_width_height(width, height) : 1,
    .sample_count = config.sample_count,
    .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
             VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    .additional_name_data = std::format("LoadedFromMemory@{}", config.path),
    .path = config.path,
  });

  transition_image_layout(buffer->get_command_buffer(),
                          image->image,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          image->get_aspect_flags(),
                          image->get_mip_levels());
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

  vkCmdCopyBufferToImage(buffer->get_command_buffer(),
                         staging_buffer->get_buffer(),
                         image->image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1,
                         &region);

  if (image->get_mip_levels() > 1) {
    image->generate_mips(buffer->get_command_buffer());
  } else {
    transition_image_layout(buffer->get_command_buffer(),
                            image->image,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            image->get_layout(),
                            image->get_aspect_flags(),
                            image->get_mip_levels());
  }
  return image;
}
auto
Image::resolve_msaa(const Image&, const CommandBuffer*) -> Core::Scope<Image>
{
  return nullptr;
}

auto
Image::reference_resolve_msaa(const Image&, const CommandBuffer*)
  -> Core::Ref<Image>
{
  return nullptr;
}

auto
Image::copy_image(const Image& source, const CommandBuffer& command_buffer)
  -> Core::Ref<Image>
{
  auto image = Image::construct(source.configuration);

  auto old_source_layout = source.get_layout();
  transition_image_layout(command_buffer.get_command_buffer(),
                          image->image,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          image->get_aspect_flags(),
                          image->get_mip_levels());
  transition_image_layout(command_buffer.get_command_buffer(),
                          source.image,
                          old_source_layout,
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          source.get_aspect_flags(),
                          source.get_mip_levels());

  VkImageCopy copy_region{};
  copy_region.srcSubresource.aspectMask = image->get_aspect_flags();
  copy_region.srcSubresource.mipLevel = 0;
  copy_region.srcSubresource.baseArrayLayer = 0;
  copy_region.srcSubresource.layerCount = 1;
  copy_region.srcOffset = { 0, 0, 0 };
  copy_region.dstSubresource.aspectMask = image->get_aspect_flags();
  copy_region.dstSubresource.mipLevel = 0;
  copy_region.dstSubresource.baseArrayLayer = 0;
  copy_region.dstSubresource.layerCount = 1;
  copy_region.dstOffset = { 0, 0, 0 };
  copy_region.extent = {
    source.configuration.width,
    source.configuration.height,
    1,
  };

  vkCmdCopyImage(command_buffer.get_command_buffer(),
                 source.image,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 image->image,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 1,
                 &copy_region);

  transition_image_layout(command_buffer.get_command_buffer(),
                          image->image,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          image->get_layout(),
                          image->get_aspect_flags(),
                          image->get_mip_levels());
  transition_image_layout(command_buffer.get_command_buffer(),
                          source.image,
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          old_source_layout,
                          source.get_aspect_flags(),
                          source.get_mip_levels());

  return image;
}

Image::Image(const ImageConfiguration& conf)
  : aspect_mask(to_aspect_mask(conf.format))
  , configuration(conf)
{
  alloc_impl = Core::make_scope<ImageImpl>();
  invalidate();
}

auto
Image::create_specific_layer_image_views(
  const std::span<const Core::u32> indices) -> void
{
  for (unsigned int index : indices) {
    auto& view_to_be_created = layer_image_views[index];
    VkImageViewCreateInfo view_create_info{};
    view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.image = image;
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_create_info.format = configuration.format;
    view_create_info.subresourceRange.aspectMask = aspect_mask;
    view_create_info.subresourceRange.levelCount = configuration.mip_levels;
    view_create_info.subresourceRange.baseArrayLayer = index;
    view_create_info.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(
      Device::the().device(), &view_create_info, nullptr, &view_to_be_created));
  }
}

auto
Image::invalidate() -> void
{
  destroy();

  create_image(
    configuration, image, alloc_impl->allocation, alloc_impl->allocation_info);

  VkImageViewCreateInfo view_create_info{};
  view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_create_info.image = image;
  view_create_info.viewType = configuration.layers > 1
                                ? VK_IMAGE_VIEW_TYPE_2D_ARRAY
                                : VK_IMAGE_VIEW_TYPE_2D;
  view_create_info.format = configuration.format;
  view_create_info.subresourceRange.aspectMask = aspect_mask;
  view_create_info.subresourceRange.levelCount = configuration.mip_levels;
  view_create_info.subresourceRange.layerCount = configuration.layers;
  VK_CHECK(vkCreateImageView(
    Device::the().device(), &view_create_info, nullptr, &view));

  sampler =
    create_sampler(configuration.min_filter,
                   configuration.mag_filter,
                   configuration.address_mode_u,
                   configuration.address_mode_v,
                   configuration.address_mode_w,
                   configuration.border_colour,
                   configuration.mip_levels > 1 ? configuration.mip_levels : 0);

  descriptor_info.imageLayout = configuration.layout;
  descriptor_info.imageView = view;
  descriptor_info.sampler = sampler;

  destroyed = false;
  invalidate_hash();

  if (!configuration.transition_directly) {
    return;
  }
  Device::the().execute_immediate([&](auto* buf) {
    transition_image_layout(buf,
                            image,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            get_layout(),
                            aspect_mask,
                            configuration.mip_levels,
                            0);
  });
}

auto
Image::generate_mips(VkCommandBuffer buf) -> void
{
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = image;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  auto mip_width = static_cast<Core::i32>(configuration.width);
  auto mip_height = static_cast<Core::i32>(configuration.height);

  for (auto i = 1U; i < configuration.mip_levels; i++) {
    transition_image_layout(buf,
                            image,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            aspect_mask,
                            1,
                            i - 1);

    VkImageBlit blit{};
    blit.srcOffsets[0] = { 0, 0, 0 };
    blit.srcOffsets[1] = {
      mip_width,
      mip_height,
      1,
    };
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = { 0, 0, 0 };
    blit.dstOffsets[1] = {
      mip_width > 1 ? mip_width / 2 : 1,
      mip_height > 1 ? mip_height / 2 : 1,
      1,
    };
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    vkCmdBlitImage(buf,
                   image,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1,
                   &blit,
                   VK_FILTER_LINEAR);

    transition_image_layout(buf,
                            image,
                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                            aspect_mask,
                            1,
                            i - 1);

    if (mip_width > 1) {
      mip_width /= 2;
    }
    if (mip_height > 1) {
      mip_height /= 2;
    }
  }

  transition_image_layout(buf,
                          image,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          get_layout(),
                          aspect_mask,
                          1,
                          configuration.mip_levels - 1);

  for (auto i = 0U; i < configuration.mip_levels; i++) {
    auto& view_to_be_created = mip_image_views[i];
    VkImageViewCreateInfo view_create_info{};
    view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.image = image;
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_create_info.format = configuration.format;
    view_create_info.subresourceRange.aspectMask = aspect_mask;
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.baseMipLevel = i;
    view_create_info.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(
      Device::the().device(), &view_create_info, nullptr, &view_to_be_created));
  }
}

auto
Image::construct(const ImageConfiguration& config) -> Core::Ref<Image>
{
  return Core::make_ref<Image>(config);
}

auto
Image::hash() -> Core::usize
{
  if (!hash_value.has_value()) {
    invalidate_hash();
  }

  CHECK(hash_value);
  return hash_value.value();
}

auto
Image::invalidate_hash() -> void
{
  static constexpr auto golden_ratio_constant = 0x9e3779b9;
  static constexpr auto hash_combine = []<typename... Ts>(Core::usize& seed,
                                                          Ts... ts) {
    ((seed ^=
      std::hash<Ts>{}(ts) + golden_ratio_constant + (seed << 6) + (seed >> 2)),
     ...);
  };

  hash_value = 0;
  hash_combine(hash_value.value(),
               configuration.width,
               configuration.height,
               configuration.format,
               configuration.sample_count,
               configuration.mip_levels,
               configuration.layers,
               configuration.usage,
               configuration.tiling,
               configuration.layout,
               configuration.min_filter,
               configuration.mag_filter,
               configuration.address_mode_u,
               configuration.address_mode_v,
               configuration.address_mode_w,
               configuration.border_colour,
               std::bit_cast<const void*>(view),
               std::bit_cast<const void*>(sampler),
               std::bit_cast<const void*>(image));
}

auto
Image::write_to_file(const std::string_view path) const -> bool
{
  std::filesystem::path file_path{ path };
  // Check parent directory exists
  if (!std::filesystem::exists(file_path.parent_path())) {
    error("Parent directory does not exist for file '{}'", path);
    return false;
  }

  auto layout = configuration.layout;
  auto aspect = aspect_mask;
  auto mip_levels = configuration.mip_levels;
  auto width = configuration.width;
  auto height = configuration.height;

  // Need to transition to general, copy into vkbuffer, copy vkbuffer data to a
  // DataBuffer, then write to file, then transition back

  VkBufferCreateInfo buffer_create_info{};
  buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_create_info.size = static_cast<VkDeviceSize>(width) *
                            static_cast<VkDeviceSize>(height) *
                            static_cast<VkDeviceSize>(4 * 4);
  buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  VkBuffer staging_buffer{};
  VmaAllocationInfo staging_allocation_info{};
  Allocator allocator{ "Image" };
  auto* allocation = allocator.allocate_buffer(
    staging_buffer,
    staging_allocation_info,
    buffer_create_info,
    {
      .usage = Usage::AUTO_PREFER_DEVICE,
      .creation = Creation::HOST_ACCESS_RANDOM_BIT | Creation::MAPPED_BIT,
    });

  Core::DataBuffer data_buffer{ width * height * 4 };
  Device::the().execute_immediate([&](auto* cmd_buffer) {
    transition_image_layout(cmd_buffer,
                            image,
                            layout,
                            VK_IMAGE_LAYOUT_GENERAL,
                            aspect,
                            mip_levels,
                            0);
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

    vkCmdCopyImageToBuffer(
      cmd_buffer, image, VK_IMAGE_LAYOUT_GENERAL, staging_buffer, 1, &region);

    const auto* mapped =
      static_cast<Core::u8*>(staging_allocation_info.pMappedData);
    data_buffer.write(mapped, width * height * 4);

    transition_image_layout(cmd_buffer,
                            image,
                            VK_IMAGE_LAYOUT_GENERAL,
                            layout,
                            aspect,
                            mip_levels,
                            0);
  });

  auto output = stbi_write_png(
    file_path.string().c_str(), width, height, 4, data_buffer.raw(), width * 4);

  allocator.deallocate_buffer(allocation, staging_buffer);
  return output != 0;
}

} // namespace Engine::Graphic
