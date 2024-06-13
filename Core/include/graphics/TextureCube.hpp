#pragma once

#include "core/Types.hpp"
#include "graphics/Image.hpp"

#include <string>

namespace Engine::Graphics {

class TextureCube
{
public:
  static auto construct(const std::string& path) -> Core::Ref<TextureCube>;

  auto get_image() const { return image; }

private:
  auto load_from_file(const std::string& path) -> bool;
  Core::Ref<Image> image;
};

} // namespace Engine::Graphics
