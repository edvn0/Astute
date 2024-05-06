#pragma once

#include "AstuteCore.hpp"

namespace Widgets {

struct Widget
{
  virtual ~Widget() = default;
  virtual auto construct() -> void {}
  virtual auto destruct() -> void {}
  virtual auto update(Engine::Core::f64 time_step) -> void {}
  virtual auto interpolate(Engine::Core::f64 superfluous_time_step) -> void {}
  virtual auto interface() -> void {}
  virtual auto handle_events(Engine::Core::Event& event) -> void {}
  virtual auto on_resize(const Engine::Core::Extent& new_extent) -> void {}
};

} // namespace Application::Widgets
