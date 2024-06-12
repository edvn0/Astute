#pragma once

#include "core/InputCodes.hpp"

#include <GLFW/glfw3.h>

namespace Engine::Core {

class Input
{
public:
  Input() = delete;

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

  static auto is_gamepad_present(Core::i32 gamepad) -> bool;
  static auto get_gamepad_name(Core::i32 gamepad) -> std::string;

  static auto get_gamepad_buttons(Core::i32 gamepad, unsigned char* buttons)
    -> bool;
  template<Core::usize Count>
  static auto get_gamepad_buttons(Core::i32 gamepad,
                                  std::array<Core::u8, Count>& buttons) -> bool
  {
    return get_gamepad_buttons(gamepad, buttons.data());
  }
  static auto get_gamepad_axes(Core::i32 gamepad, float* axes) -> bool;

  template<Core::usize Count>
  static auto get_gamepad_axes(Core::i32 gamepad,
                               std::array<Core::f32, Count>& axes) -> bool
  {
    return get_gamepad_axes(gamepad, axes.data());
  }

  static auto released(KeyCode code) -> bool;
  static auto released(MouseCode code) -> bool;
  static auto pressed(MouseCode code) -> bool;
  static auto mouse_position() -> std::tuple<f32, f32>;
  static auto pressed(KeyCode code) -> bool;
  static auto initialise(GLFWwindow* win) { window = win; }

private:
  static inline GLFWwindow* window;
};

} // namespace Engine::Core
