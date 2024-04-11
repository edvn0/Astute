#pragma once

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

  Application(Configuration);
  virtual ~Application();

  auto run() -> i32;

  virtual auto update(f64) -> void{};
  virtual auto interpolate(f64) -> void{};

private:
  Configuration config{};
  Statistics statistics{};

  Scope<Graphics::Window> window;
};

} // namespace Engine::Core
