#include "pch/CorePCH.hpp"

#include "core/Clock.hpp"

#include <GLFW/glfw3.h>

namespace Engine::Core {

auto
Clock::now() -> f64
{
  return glfwGetTime();
}

auto
Clock::now_ms() -> f64
{
  return glfwGetTime() * 1000.0;
}

} // namespace Engine::Core
