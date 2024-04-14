#pragma once

namespace Engine::Graphics {

class Renderer
{
public:
  auto begin_frame() -> void;
  auto end_frame() -> void;
};

}
