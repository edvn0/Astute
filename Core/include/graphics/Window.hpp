#pragma once

#include "graphics/Forward.hpp"

#include "core/Event.hpp"
#include "core/Types.hpp"

#include <GLFW/glfw3.h>
#include <functional>
#include <vulkan/vulkan.h>

namespace Engine::Graphics {

class Window
{
public:
  struct Configuration
  {
    Core::Extent size{ 1600, 900 };
    const bool is_headless{ false };
    const bool start_fullscreen{ false };
    bool is_fullscreen{ false };
    const bool is_vsync{ false };

    Core::u32 windowed_width{ 1600 };
    Core::u32 windowed_height{ 900 };
    Core::u32 windowed_position_x{ 0 };
    Core::u32 windowed_position_y{ 0 };
  };

  explicit Window(Configuration);
  ~Window();

  auto get_surface() { return surface; }
  auto get_window() const -> const GLFWwindow* { return window; }
  auto should_close() -> bool;
  auto update() -> void;

  auto begin_frame() -> void;
  auto present() -> void;

  auto set_event_handler(std::function<void(Core::Event&)>&&) -> void;
  auto toggle_fullscreen() -> void;
  auto close() -> void;

private:
  Core::Scope<Swapchain> swapchain;

  Configuration configuration{};
  GLFWwindow* window;
  VkSurfaceKHR surface;

  struct UserPointer
  {
    bool was_resized{ false };
    std::function<void(Core::Event&)> event_callback = [](auto&) {};
  };
  UserPointer user_data{};
};

} // namespace Core
