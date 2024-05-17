#pragma once

#include "core/Types.hpp"

#include "graphics/Image.hpp"

namespace Engine::Graphics {

class TextureGenerator
{
public:
  static auto simplex_noise(Core::u32 width, Core::u32 height)
    -> Core::Ref<Image>;
};

} // namespace Engine::Graphics
