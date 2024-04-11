#pragma once

#include "core/InputCodes.hpp"

#include <GLFW/glfw3.h>

namespace Engine::Core {

class Input
{
public:
  template<KeyCode K>
  static auto pressed() -> bool
  {
    return pressed(K);
  }
  template<MouseCode M>
  static auto pressed() -> bool
  {
    return pressed(M);
  }
  template<KeyCode K>
  static auto released() -> bool
  {
    return released(K);
  }
  template<MouseCode M>
  static auto released() -> bool
  {
    return released(M);
  }

  static auto released(KeyCode code) -> bool;
  static auto released(MouseCode code) -> bool;
  static auto pressed(MouseCode code) -> bool;
  static auto mouse_position() -> std::tuple<f32, f32>;
  static auto pressed(KeyCode code) -> bool;
  static auto initialise(GLFWwindow* win) { window = win; }

private:
  Input() = delete;
  static inline GLFWwindow* window;
};

} // namespace Engine::Core
