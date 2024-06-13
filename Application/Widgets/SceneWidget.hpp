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
  auto update(Engine::Core::f64) -> void override {}
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
};

} // namespace Application::Widgets
