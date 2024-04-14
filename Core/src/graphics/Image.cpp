#include "pch/CorePCH.hpp"

#include "graphics/Image.hpp"

namespace Engine::Graphics {

auto
Image::descriptor_info() -> VkDescriptorImageInfo
{
  VkDescriptorImageInfo info = {};
  info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  info.imageView = view;
  info.sampler = sampler;
  return info;
}

} // namespace Engine::Graphic
