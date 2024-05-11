#include "pch/CorePCH.hpp"

#include "graphics/Device.hpp"
#include "graphics/Instance.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

#include "core/Input.hpp"
#include "core/Logger.hpp"

namespace Engine::Graphics {

static constexpr auto default_file_path = "window_size_and_position.txt";

Window::Window(Configuration config)
  : configuration(config)
{
  if (glfwInit() != GLFW_TRUE) {
    throw Core::InvalidInitialisationException{
      "Could not initalise GLFW.",
    };
  }

  if (!glfwVulkanSupported()) {
    error("Vulkan not supported");
    throw Core::InvalidInitialisationException{
      "Vulkan not supported",
    };
  }

  auto load_previous_window_size_and_position_from_file =
    [](std::string_view file_path) {
      std::ifstream file(file_path.data());
      if (!file.is_open()) {
        return std::make_tuple(1600U, 900U, 0, 0);
      }

      Core::u32 width{};
      Core::u32 height{};
      Core::i32 x{};
      Core::i32 y{};
      file >> width >> height >> x >> y;
      return std::make_tuple(width, height, x, y);
    };

  auto [loaded_width, loaded_height, loaded_x, loaded_y] =
    load_previous_window_size_and_position_from_file(default_file_path);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  if (loaded_width != 0 && loaded_height != 0) {
    configuration.size = {
      loaded_width,
      loaded_height,
    };
  }

  auto&& [width, height] = configuration.size.as<Core::i32>();

  if (configuration.start_fullscreen) {
    auto* monitor = glfwGetPrimaryMonitor();
    const auto* mode = glfwGetVideoMode(monitor);

    glfwWindowHint(GLFW_DECORATED, false);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    window =
      glfwCreateWindow(mode->width, mode->height, "Astute", monitor, nullptr);
    configuration.is_fullscreen = true;
  } else {
    window = glfwCreateWindow(width, height, "Astute", nullptr, nullptr);
  }

  if (loaded_x != 0 || loaded_y != 0) {
    glfwSetWindowPos(window, loaded_x, loaded_y);
  }

  Core::Input::initialise(window);

  glfwCreateWindowSurface(
    Instance::the().instance(), window, nullptr, &surface);

  Device::initialise(surface);

  swapchain = Core::make_scope<Swapchain>(this);
  swapchain->initialise(this, get_surface());
  swapchain->create(configuration.size, configuration.is_vsync);

  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(
    window, +[](GLFWwindow* win, Core::i32 w, Core::i32 h) {
      auto& self = *static_cast<Window*>(glfwGetWindowUserPointer(win));
      self.configuration.size = {
        static_cast<Core::u32>(w),
        static_cast<Core::u32>(h),
      };
      self.user_data.was_resized = true;
    });

  glfwSetKeyCallback(
    window,
    +[](
       GLFWwindow* win, Core::i32 key, Core::i32, Core::i32 action, Core::i32) {
      const auto& self = *static_cast<Window*>(glfwGetWindowUserPointer(win));
      switch (action) {
        case GLFW_PRESS: {
          Core::KeyPressedEvent event{ key, 0 };
          self.user_data.event_callback(event);
          break;
        }
        case GLFW_RELEASE: {
          Core::KeyReleasedEvent event{ key };
          self.user_data.event_callback(event);
          break;
        }
        case GLFW_REPEAT: {
          Core::KeyPressedEvent event{ key, 1 };
          self.user_data.event_callback(event);
          break;
        }
        default: {
          error("Unknown key action: {}", action);
          break;
        }
      }
    });
  glfwSetWindowSizeCallback(
    window, +[](GLFWwindow* win, Core::i32 w, Core::i32 h) {
      auto& self = *static_cast<Window*>(glfwGetWindowUserPointer(win));
      self.configuration.size = {
        static_cast<Core::u32>(w),
        static_cast<Core::u32>(h),
      };
      self.user_data.was_resized = true;

      Core::WindowResizeEvent event{ w, h };
      self.user_data.event_callback(event);
    });

  glfwSetScrollCallback(
    window, +[](GLFWwindow* win, Core::f64 x, Core::f64 y) {
      const auto& self = *static_cast<Window*>(glfwGetWindowUserPointer(win));
      Core::MouseScrolledEvent event{ static_cast<float>(x),
                                      static_cast<float>(y) };
      self.user_data.event_callback(event);
    });

  glfwSetMouseButtonCallback(
    window, +[](GLFWwindow* win, int button, int action, int) {
      const auto& self = *static_cast<Window*>(glfwGetWindowUserPointer(win));

      // Assuming you have a way to get the current mouse position
      double mouse_x;
      double mouse_y;
      glfwGetCursorPos(win, &mouse_x, &mouse_y);

      if (action == GLFW_PRESS) {
        Core::MouseButtonPressedEvent event(
          static_cast<Core::MouseCode>(button),
          static_cast<Core::f32>(mouse_x),
          static_cast<Core::f32>(mouse_y));
        self.user_data.event_callback(event);
      }
      if (action == GLFW_RELEASE) {
        Core::MouseButtonReleasedEvent event(
          static_cast<Core::MouseCode>(button),
          static_cast<Core::f32>(mouse_x),
          static_cast<Core::f32>(mouse_y));
        self.user_data.event_callback(event);
      }
    });

  glfwSetCursorPosCallback(
    window, +[](GLFWwindow* win, double x, double y) {
      const auto& self = *static_cast<Window*>(glfwGetWindowUserPointer(win));
      Core::MouseMovedEvent event{
        static_cast<float>(x),
        static_cast<float>(y),
      };
      self.user_data.event_callback(event);
    });
}

auto
Window::toggle_fullscreen() -> void
{
  if (!configuration.is_fullscreen) {
    Core::i32 w{};
    Core::i32 h{};
    Core::i32 x{};
    Core::i32 y{};
    glfwGetWindowSize(window, &w, &h);
    glfwGetWindowPos(window, &x, &y);

    configuration.windowed_width = static_cast<Core::u32>(w);
    configuration.windowed_height = static_cast<Core::u32>(h);
    configuration.windowed_position_x = static_cast<Core::u32>(x);
    configuration.windowed_position_y = static_cast<Core::u32>(y);

    auto* monitor = glfwGetPrimaryMonitor();
    const auto* mode = glfwGetVideoMode(monitor);

    glfwSetWindowMonitor(
      window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    configuration.is_fullscreen = true;
    user_data.was_resized = true;
  } else {
    glfwSetWindowMonitor(window,
                         nullptr,
                         configuration.windowed_position_x,
                         configuration.windowed_position_y,
                         configuration.windowed_width,
                         configuration.windowed_height,
                         0);
    configuration.is_fullscreen = false;
    user_data.was_resized = true;
  }
}

Window::~Window()
{
  try {

    // Write window size and position to file
    std::ofstream file(default_file_path);
    if (file.is_open()) {
      Core::i32 width{};
      Core::i32 height{};
      glfwGetWindowPos(window, &width, &height);
      file << configuration.size.width << ' ' << configuration.size.height
           << ' ' << width << ' ' << height;
    }
  } catch (const std::exception& e) {
    error("Failed to write window size and position to file: {}", e.what());
  }

  swapchain.reset();
  glfwDestroyWindow(window);
  glfwTerminate();
  vkDestroySurfaceKHR(Instance::the().instance(), surface, nullptr);
}

auto
Window::should_close() -> bool
{
  return glfwWindowShouldClose(window);
}

auto
Window::close() -> void
{
  glfwSetWindowShouldClose(window, GLFW_TRUE);
}

auto
Window::update() -> void
{
  glfwPollEvents();
}

auto
Window::present() -> void
{
  swapchain->present();
}

auto
Window::begin_frame() -> bool
{
  return swapchain->begin_frame();
}

auto
Window::set_event_handler(std::function<void(Core::Event&)>&& func) -> void
{
  user_data.event_callback = std::move(func);
}

}
