#pragma once

#include "core/Event.hpp"
#include "core/Types.hpp"

#include "graphics/Forward.hpp"

namespace Engine::Core {

class Application
{
public:
  struct Configuration
  {
    const bool headless{ false };
    const Extent size{ 1920, 1080 };
    const bool fullscreen{ false };
  };

  struct Statistics
  {
    f64 frame_time{ 0.0 };
    f64 frames_per_seconds{ 0.0 };
  };

  explicit Application(Configuration);
  virtual ~Application();

  auto run() -> i32;

  virtual auto update(f64) -> void {}
  virtual auto interpolate(f64) -> void {}
  virtual auto handle_events(Event&) -> void {}
  virtual auto interface() -> void {}
  virtual auto on_resize(const Extent&) -> void {}

protected:
  auto get_statistics() const -> const Statistics& { return statistics; }
  auto get_statistics() -> Statistics& { return statistics; }
  auto get_configuration() const -> const Configuration& { return config; }
  auto get_configuration() -> Configuration& { return config; }
  auto get_window() const -> const Graphics::Window& { return *window; }
  auto get_window() -> Graphics::Window& { return *window; }

private:
  Configuration config{};
  Statistics statistics{};

  Scope<Graphics::Window> window;
  auto forward_incoming_events(Event&) -> void;
};

} // namespace Engine::Core
