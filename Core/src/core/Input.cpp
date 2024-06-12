#include "pch/CorePCH.hpp"

#include "core/Input.hpp"

#define GLFW_CHECK(x) x == GLFW_TRUE

namespace Engine::Core {
auto
Input::released(KeyCode code) -> bool
{
  return glfwGetKey(window, static_cast<int>(code)) == GLFW_RELEASE;
}
auto
Input::released(MouseCode code) -> bool
{
  return glfwGetMouseButton(window, static_cast<int>(code)) == GLFW_RELEASE;
}
auto
Input::pressed(MouseCode code) -> bool
{
  return glfwGetMouseButton(window, static_cast<int>(code)) == GLFW_PRESS;
}
auto
Input::mouse_position() -> std::tuple<f32, f32>
{
  double x{};
  double y{};
  glfwGetCursorPos(window, &x, &y);
  return { static_cast<f32>(x), static_cast<f32>(y) };
}

auto
Input::pressed(KeyCode code) -> bool
{
  return glfwGetKey(window, static_cast<int>(code)) == GLFW_PRESS;
}

auto
Input::is_gamepad_present(int gamepad) -> bool
{
  return GLFW_CHECK(glfwJoystickPresent(gamepad)) &&
         GLFW_CHECK(glfwJoystickIsGamepad(gamepad));
}

auto
Input::get_gamepad_name(int gamepad) -> std::string
{
  if (is_gamepad_present(gamepad)) {
    return glfwGetGamepadName(gamepad);
  }
  return {};
}

auto
Input::get_gamepad_buttons(int gamepad, unsigned char* buttons) -> bool
{
  if (is_gamepad_present(gamepad)) {
    GLFWgamepadstate state{};
    if (GLFW_CHECK(glfwGetGamepadState(gamepad, &state))) {
      std::memcpy(buttons, state.buttons, sizeof(state.buttons));
      return true;
    }
  }
  return false;
}

auto
Input::get_gamepad_axes(int gamepad, float* axes) -> bool
{
  if (is_gamepad_present(gamepad)) {
    GLFWgamepadstate state{};
    if (GLFW_CHECK(glfwGetGamepadState(gamepad, &state))) {
      std::memcpy(axes, state.axes, sizeof(state.axes));
      return true;
    }
  }
  return false;
}

}
