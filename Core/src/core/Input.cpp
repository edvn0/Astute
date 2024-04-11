#include "pch/CorePCH.hpp"

#include "core/Input.hpp"

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
  double x{}, y{};
  glfwGetCursorPos(window, &x, &y);
  return { static_cast<f32>(x), static_cast<f32>(y) };
}

auto
Input::pressed(KeyCode code) -> bool
{
  return glfwGetKey(window, static_cast<int>(code)) == GLFW_PRESS;
}

}