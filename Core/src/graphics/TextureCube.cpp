#include "pch/CorePCH.hpp"

#include "graphics/TextureCube.hpp"

#include "core/DataBuffer.hpp"
#include "core/Verify.hpp"
#include "graphics/Allocator.hpp"
#include "graphics/GPUBuffer.hpp"
#include "graphics/Image.hpp"
#include "logging/Logger.hpp"

#include <cassert>
#include <ktx.h>
#include <ktxvulkan.h>

namespace Engine::Graphics {

static constexpr auto
to_string(ktx_error_code_e e) -> std::string_view
{
  switch (e) {
    case KTX_SUCCESS:
      return "KTX_SUCCESS";
    case KTX_FILE_DATA_ERROR:
      return "KTX_FILE_DATA_ERROR";
    case KTX_FILE_ISPIPE:
      return "KTX_FILE_ISPIPE";
    case KTX_FILE_OPEN_FAILED:
      return "KTX_FILE_OPEN_FAILED";
    case KTX_FILE_OVERFLOW:
      return "KTX_FILE_OVERFLOW";
    case KTX_FILE_READ_ERROR:
      return "KTX_FILE_READ_ERROR";
    case KTX_FILE_SEEK_ERROR:
      return "KTX_FILE_SEEK_ERROR";
    case KTX_FILE_UNEXPECTED_EOF:
      return "KTX_FILE_UNEXPECTED_EOF";
    case KTX_FILE_WRITE_ERROR:
      return "KTX_FILE_WRITE_ERROR";
    case KTX_GL_ERROR:
      return "KTX_GL_ERROR";
    case KTX_INVALID_OPERATION:
      return "KTX_INVALID_OPERATION";
    case KTX_INVALID_VALUE:
      return "KTX_INVALID_VALUE";
    case KTX_NOT_FOUND:
      return "KTX_NOT_FOUND";
    case KTX_OUT_OF_MEMORY:
      return "KTX_OUT_OF_MEMORY";
    case KTX_TRANSCODE_FAILED:
      return "KTX_TRANSCODE_FAILED";
    case KTX_UNKNOWN_FILE_FORMAT:
      return "KTX_UNKNOWN_FILE_FORMAT";
    case KTX_UNSUPPORTED_TEXTURE_TYPE:
      return "KTX_UNSUPPORTED_TEXTURE_TYPE";
    case KTX_UNSUPPORTED_FEATURE:
      return "KTX_UNSUPPORTED_FEATURE";
    case KTX_LIBRARY_NOT_LINKED:
      return "KTX_LIBRARY_NOT_LINKED";
    case KTX_DECOMPRESS_LENGTH_ERROR:
      return "KTX_DECOMPRESS_LENGTH_ERROR";
    case KTX_DECOMPRESS_CHECKSUM_ERROR:
      return "KTX_DECOMPRESS_CHECKSUM_ERROR";
    default:
      return "Missing";
  }
}

struct Mipmap final
{
  Core::u32 level = 0;
  Core::u32 offset = 0;
  VkExtent3D extent = { 0, 0, 0 };
};

struct CallbackData final
{
  ktxTexture2* texture;
  std::vector<Mipmap>* mipmaps;
};

auto
TextureCube::load_from_file(const std::string& path) -> bool
{
  std::filesystem::path p(path);
  if (!std::filesystem::exists(p)) {
    error("File does not exist: {}", path);
    return false;
  }

  ktxTexture* loaded_texture = nullptr;
  ktxResult result =
    ktxTexture_CreateFromNamedFile(path.c_str(), 0x0, &loaded_texture);
  if (result != KTX_SUCCESS) {
    auto error_code = to_string(result);
    error("Failed to load KTX file: {}. Code: {}", path, error_code);
    return false;
  }

  auto output_image = Core::make_ref<Image>();

  // Get properties required for using and upload texture data from the ktx
  // texture object
  auto width = loaded_texture->baseWidth;
  auto height = loaded_texture->baseHeight;
  auto mipLevels = loaded_texture->numLevels;
  auto layers = loaded_texture->numLayers;
  output_image->configuration.width = width;
  output_image->configuration.height = height;
  output_image->configuration.mip_levels = mipLevels;
  output_image->configuration.layers = layers;

  ktx_size_t ktxTextureSize =
    ktxTexture_GetDataSize(ktxTexture(loaded_texture));
  std::vector<Core::u8> data;
  data.resize(ktxTextureSize);
  ktxTexture_LoadImageData(loaded_texture, data.data(), ktxTextureSize);

  StagingBuffer staging_buffer{ std::span{ data } };
  auto device = Device::the().device();

  // Create optimal tiled target image
  VkImageCreateInfo imageCreateInfo = {};
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = // Format from the KTX file
    static_cast<VkFormat>(ktxTexture_GetVkFormat(loaded_texture));
  imageCreateInfo.mipLevels = mipLevels;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageCreateInfo.extent = {
    width,
    height,
    1,
  };
  imageCreateInfo.usage =
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  // Cube faces count as array layers in Vulkan
  imageCreateInfo.arrayLayers = 6;
  // This flag is required for cube map images
  imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

  output_image->allocate(imageCreateInfo);

  std::vector<VkBufferImageCopy> bufferCopyRegions;
  for (uint32_t face = 0; face < 6; face++) {
    for (uint32_t level = 0; level < output_image->get_mip_levels(); level++) {
      // Calculate offset into staging buffer for the current mip level and face
      ktx_size_t ktx_offset;
      ktxTexture_GetImageOffset(
        ktxTexture(loaded_texture), level, 0, face, &ktx_offset);
      VkBufferImageCopy bufferCopyRegion = {};
      bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      bufferCopyRegion.imageSubresource.mipLevel = level;
      bufferCopyRegion.imageSubresource.baseArrayLayer = face;
      bufferCopyRegion.imageSubresource.layerCount = 1;
      bufferCopyRegion.imageExtent.width = loaded_texture->baseWidth >> level;
      bufferCopyRegion.imageExtent.height = loaded_texture->baseHeight >> level;
      bufferCopyRegion.imageExtent.depth = 1;
      bufferCopyRegion.bufferOffset = ktx_offset;
      bufferCopyRegions.push_back(bufferCopyRegion);
    }
  }

  // Image barrier for optimal image (target)
  // Set initial layout for all array layers (faces) of the optimal (target)
  // tiled texture
  VkImageSubresourceRange subresourceRange = {};
  subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subresourceRange.baseMipLevel = 0;
  subresourceRange.levelCount = output_image->get_mip_levels();
  subresourceRange.layerCount = 6;

  Device::the().execute_immediate([&](VkCommandBuffer cmd) {
    transition_image_layout(cmd,
                            output_image->image,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            subresourceRange);
    vkCmdCopyBufferToImage(cmd,
                           staging_buffer.get_buffer(),
                           output_image->image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           static_cast<uint32_t>(bufferCopyRegions.size()),
                           bufferCopyRegions.data());

    // Change texture image layout to shader read after all faces have been
    // copied
    output_image->configuration.layout =
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    transition_image_layout(cmd,
                            output_image->image,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            output_image->configuration.layout,
                            subresourceRange);
  });

  // Create sampler
  VkSamplerCreateInfo sampler{};
  sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler.magFilter = VK_FILTER_LINEAR;
  sampler.minFilter = VK_FILTER_LINEAR;
  sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler.addressModeV = sampler.addressModeU;
  sampler.addressModeW = sampler.addressModeU;
  sampler.mipLodBias = 0.0f;
  sampler.compareOp = VK_COMPARE_OP_NEVER;
  sampler.minLod = 0.0f;
  sampler.maxLod = static_cast<float>(output_image->get_mip_levels());
  sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  sampler.maxAnisotropy = 1.0f;
  sampler.maxAnisotropy = 16;
  sampler.anisotropyEnable = VK_TRUE;
  VK_CHECK(vkCreateSampler(device, &sampler, nullptr, &output_image->sampler));

  // Create image view
  VkImageViewCreateInfo view = {};
  view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  // Cube map view type
  view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
  view.format = output_image->configuration.format;
  view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
  // 6 array layers (faces)
  view.subresourceRange.layerCount = 6;
  // Set number of mip levels
  view.subresourceRange.levelCount = output_image->get_mip_levels();
  view.image = output_image->image;
  VK_CHECK(vkCreateImageView(device, &view, nullptr, &output_image->view));

  // Clean up staging resources
  staging_buffer.destroy();
  ktxTexture_Destroy(loaded_texture);

  image = output_image;

  image->descriptor_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image->descriptor_info.imageView = image->view;
  image->descriptor_info.sampler = image->sampler;
  return true;
}

auto
TextureCube::construct(const std::string& path) -> Core::Ref<TextureCube>
{
  auto cube = Core::make_ref<TextureCube>();
  if (!cube->load_from_file(path)) {
    error("Failed to load texture cube from file: {}", path);
    return nullptr;
  }

  return cube;
}

} // namespace Engine::Graphics
