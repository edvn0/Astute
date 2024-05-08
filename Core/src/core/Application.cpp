#include "pch/CorePCH.hpp"

#include "core/Application.hpp"
#include "core/Clock.hpp"
#include "core/Logger.hpp"

#include "graphics/Allocator.hpp"
#include "graphics/DescriptorResource.hpp"
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
  dispatcher.dispatch<WindowResizeEvent>([this](const WindowResizeEvent& ev) {
    const BasicExtent extent{ ev.get_width(), ev.get_height() };
    on_resize(extent.as<u32>());
    return true;
  });

  if (event.handled)
    return;
  handle_events(event);
}

Application::Application(const Configuration& conf)
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

  interface_system = make_scope<Graphics::InterfaceSystem>(*window);

  construct();

  while (!window->should_close()) {
    window->update();
    if (!window->begin_frame()) {
      info("Could not begin this frame.");
      continue;
    }

    Graphics::DescriptorResource::the().begin_frame();

    auto current_frame_time = Clock::now();
    auto frame_duration = current_frame_time - last_frame_time;
    last_frame_time = current_frame_time;
    accumulator += frame_duration;

    while (accumulator >= delta_time) {
      update(delta_time);
      accumulator -= delta_time;
    }
    interpolate(accumulator / delta_time);

    render();

    interface_system->begin_frame();
    interface();
    interface_system->end_frame();

    window->present();

    ++frame_count;
    if (auto current_second_time = Clock::now();
        current_second_time - last_fps_time >= 1) {
      statistics.frames_per_seconds = frame_count;
      frame_count = 0;
      last_fps_time = current_second_time;
      statistics.frame_time = frame_duration * 1000.0;

      info("Frametime: {:.5f}ms. FPS: {}Hz",
           statistics.frame_time,
           statistics.frames_per_seconds);
    }

    Graphics::DescriptorResource::the().end_frame();

    std::scoped_lock lock(post_frame_mutex);
    for (auto& func : post_frame_funcs) {
      func();
    }
    post_frame_funcs.clear();
  }

  interface_system.reset();

  for (const auto& func : deferred_destruction) {
    func();
  }

  Graphics::DescriptorResource::the().destroy();
  vkDeviceWaitIdle(Graphics::Device::the().device());

  destruct();

  info("Exiting Astute Engine.");

  return 0;
}

auto
Application::on_resize(const Extent& new_size) -> void
{
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
Application::get_image_count() const -> u32
{
  return window->get_swapchain().get_image_count();
}

auto
Application::get_swapchain() const -> const Graphics::Swapchain&
{
  return window->get_swapchain();
}

auto
Application::get_swapchain() -> Graphics::Swapchain&
{
  return window->get_swapchain();
}

} // namespace Core
