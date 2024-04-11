#include "pch/CorePCH.hpp"

#include "core/Application.hpp"
#include "core/Clock.hpp"
#include "core/Logger.hpp"

#include "graphics/Device.hpp"
#include "graphics/Instance.hpp"
#include "graphics/Window.hpp"

namespace Engine::Core {

Application::Application(Configuration conf)
  : config(conf)
{
  info("Astute Engine initialisation.");

  window = make_scope<Graphics::Window>(Graphics::Window::Configuration{
    .size = config.size,
    .start_fullscreen = config.fullscreen,
    .is_fullscreen = config.fullscreen,
  });

  (void)Graphics::Instance::initialise();
  (void)Graphics::Device::initialise(window->get_surface());
}

Application::~Application()
{
  window = nullptr;
  Graphics::Device::the().destroy();
  Graphics::Instance::the().destroy();
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

} // namespace Core
