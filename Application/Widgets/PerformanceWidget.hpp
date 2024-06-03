#pragma once

#include "AstuteCore.hpp"

#include "Widget.hpp"

namespace Widgets {

struct PerformanceMeasurement
{
  Engine::Core::f64 frame_time;
  Engine::Core::f64 fps;
};

class PerformanceWidget : public Widget
{
public:
  PerformanceWidget()
  {
    statistics.fill({
      .frame_time = 1.0 / 60.0,
      .fps = 60.0,
    });
  }
  ~PerformanceWidget() override = default;

  auto construct() -> void override {}
  auto destruct() -> void override {}
  auto update(Engine::Core::f64) -> void override;
  auto interpolate(Engine::Core::f64) -> void override {}
  auto interface() -> void override;
  auto handle_events(Engine::Core::Event&) -> void override {}
  auto on_resize(const Engine::Core::Extent&) -> void override {}

  auto set_current_scene(Engine::Core::Ref<Engine::Core::Scene> new_scene)
  {
    current_scene = std::move(new_scene);
  }
  auto set_selected_entity(Engine::Core::Ref<entt::entity> new_entity)
  {
    current_entity = std::move(new_entity);
  }

private:
  Engine::Core::Ref<Engine::Core::Scene> current_scene;
  Engine::Core::Ref<entt::entity> current_entity{ nullptr };
  static constexpr auto target_framerate = 60ULL;
  static constexpr auto buffer_size = target_framerate * 10;
  std::array<PerformanceMeasurement, buffer_size * 10> statistics;

  Engine::Core::u32 current_index{ 0 };
};

} // namespace Application::Widgets
