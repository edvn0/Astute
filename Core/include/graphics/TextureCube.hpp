#pragma once

#include "core/Types.hpp"

#include <string>

namespace Engine::Graphics {

class TextureCube
{
public:
  static auto construct(const std::string& path) -> Core::Ref<TextureCube>;

private:
    auto load_from_file(const std::string& path) -> bool;
};

} // namespace Engine::Graphics
