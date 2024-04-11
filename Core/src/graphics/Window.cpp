#include "pch/CorePCH.hpp"

#include "graphics/Device.hpp"
#include "graphics/Instance.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

#include "core/Logger.hpp"

namespace Engine::Graphics {

class InvalidInitialisationException : public std::runtime_error
{
public:
  InvalidInitialisationException(std::string_view reason)
    : std::runtime_error(reason.data())
  {
  }
};

Window::Window(Configuration config)
  : configuration(config)
{
  if (glfwInit() != GLFW_TRUE) {
    throw InvalidInitialisationException{
      "Could not initalise GLFW.",
    };
  }

  if (!glfwVulkanSupported()) {
    error("Vulkan not supported");
    throw InvalidInitialisationException{
      "Vulkan not supported",
    };
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
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
    auto&& [width, height] = configuration.size.as<Core::i32>();
    window = glfwCreateWindow(width, height, "VkGPGPU", nullptr, nullptr);
  }
  // Input::initialise(window);

  glfwCreateWindowSurface(
    Instance::the().instance(), window, nullptr, &surface);

  Device::initialise(surface);

  swapchain = Core::make_scope<Swapchain>();
  swapchain->initialise(this, get_surface());
  swapchain->create(configuration.size, configuration.is_vsync);

  glfwSetKeyCallback(window,
                     [](auto* win, auto key, auto b, auto c, auto d) -> void {
                       if (key == GLFW_KEY_ESCAPE) {
                         glfwSetWindowShouldClose(win, GLFW_TRUE);
                         return;
                       }
                     });
}

Window::~Window()
{
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
Window::begin_frame() -> void
{
  swapchain->begin_frame();
}

}
