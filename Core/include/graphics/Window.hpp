#pragma once

#include "graphics/Forward.hpp"

#include "core/Types.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

namespace Engine::Graphics {

class Window
{
public:
  struct Configuration
  {
    const Core::Extent size{ 1600, 900 };
    const bool is_headless{ false };
    const bool start_fullscreen{ false };
    bool is_fullscreen{ false };
    const bool is_vsync{ false };
  };

  Window(Configuration);
  ~Window();

  auto get_surface() { return surface; }
  auto get_window() const -> const GLFWwindow* { return window; }
  auto should_close() -> bool;
  auto update() -> void;

  auto begin_frame() -> void;
  auto present() -> void;

private:
  Core::Scope<Swapchain> swapchain;

  Configuration configuration{};
  GLFWwindow* window;
  VkSurfaceKHR surface;
};

} // namespace Core
