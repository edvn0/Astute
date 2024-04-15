#pragma once

#include "core/Types.hpp"

#include "core/Forward.hpp"

namespace Engine::Graphics {

class Shader
{
public:
  Shader(std::string_view vertex_path, std::string_view fragment_path);
  Shader(std::string_view compute_path);
  ~Shader();
};

} // namespace Engine::Graphics
