#include "pch/CorePCH.hpp"

#include "graphics/Image.hpp"

namespace Engine::Graphics {

auto
Image::get_descriptor_info() const -> const VkDescriptorImageInfo&
{
  return descriptor_info;
}

} // namespace Engine::Graphic
