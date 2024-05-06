#pragma once

#include "AstuteCore.hpp"

#include "Widget.hpp"

namespace Widgets {

class SceneWidget : public Widget
{
public:
  SceneWidget() = default;
  ~SceneWidget() override = default;

  auto construct() -> void override {}
  auto destruct() -> void override {}
  auto update(Engine::Core::f64 time_step) -> void override {}
  auto interpolate(Engine::Core::f64 superfluous_time_step) -> void override {}
  auto interface() -> void override;
  auto handle_events(Engine::Core::Event& event) -> void override {}
  auto on_resize(const Engine::Core::Extent& new_extent) -> void override {}
};

} // namespace Application::Widgets