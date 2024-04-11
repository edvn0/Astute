#include "pch/CorePCH.hpp"

#include "core/Application.hpp"
#include "core/Clock.hpp"
#include "core/Logger.hpp"

#include "graphics/Allocator.hpp"
#include "graphics/Device.hpp"
#include "graphics/Instance.hpp"
#include "graphics/InterfaceSystem.hpp"
#include "graphics/Swapchain.hpp"
#include "graphics/Window.hpp"

namespace Engine::Core {

auto
Application::forward_incoming_events(Event& event) -> void
{
  EventDispatcher dispatcher(event);
  dispatcher.dispatch<WindowResizeEvent>([this](WindowResizeEvent& event) {
    const BasicExtent extent{ event.get_width(), event.get_height() };
    on_resize(extent.as<u32>());
    return true;
  });

  if (event.handled)
    return;
  handle_events(event);
}

Application::Application(Configuration conf)
  : config(conf)
{
  info("Astute Engine initialisation.");

  window = make_scope<Graphics::Window>(Graphics::Window::Configuration{
    .size = config.size,
    .start_fullscreen = config.fullscreen,
    .is_fullscreen = config.fullscreen,
  });
  window->set_event_handler(
    [this](Event& event) { forward_incoming_events(event); });

  Graphics::Allocator::construct();

  interface_system = make_scope<Graphics::InterfaceSystem>(*window);

  instance = this;
}

Application::~Application()
{
  Graphics::Allocator::destroy();
  interface_system.reset();
  window.reset();
  Graphics::Device::destroy();
  Graphics::Instance::destroy();
}

auto
Application::run() -> i32
{
  auto last_frame_time = Clock::now();
  auto last_fps_time = last_frame_time;
  constexpr auto delta_time = 1.0 / 60.0;
  auto accumulator = 0.0;
  i32 frame_count = 0;

  while (!window->should_close()) {
    window->update();
    window->begin_frame();
    auto current_frame_time = Clock::now();
    double frame_duration = current_frame_time - last_frame_time;
    last_frame_time = current_frame_time;
    accumulator += frame_duration;

    while (accumulator >= delta_time) {
      update(delta_time);
      accumulator -= delta_time;
    }
    interpolate(accumulator / delta_time);

    interface_system->begin_frame();
    interface();
    interface_system->end_frame();

    window->present();

    ++frame_count;
    auto current_second_time = Clock::now();
    if (current_second_time - last_fps_time >= 1) {
      statistics.frames_per_seconds = frame_count;
      frame_count = 0;
      last_fps_time = current_second_time;
      statistics.frame_time = frame_duration * 1000.0;

      info("Frametime: {:.5f}ms. FPS: {}Hz",
           statistics.frame_time,
           statistics.frames_per_seconds);
    }
  }

  info("Exiting Astute Engine.");

  return 0;
}

auto
Application::the() -> Application&
{
  assert(instance != nullptr && "Application instance is null.");
  return *instance;
}

auto
Application::current_frame_index() const -> u32
{
  return window->get_swapchain().get_current_buffer_index();
}

auto
Application::get_swapchain() const -> const Graphics::Swapchain&
{
  return window->get_swapchain();
}

} // namespace Core
