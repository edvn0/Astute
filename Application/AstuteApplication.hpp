#pragma once

#include "AstuteCore.hpp"

using namespace Engine::Core;
using namespace Engine::Graphics;

#include "Widgets/PerformanceWidget.hpp"
#include "Widgets/SceneWidget.hpp"

enum class SceneState : u8
{
  Edit = 0,
  Play = 1,
  Pause = 2,
  Simulate = 3
};
enum class GizmoState : u8
{
  Translate,
  Rotate,
  Scale
};

class AstuteApplication : public Application
{
public:
  ~AstuteApplication() override;
  explicit AstuteApplication(const Application::Configuration&);

  auto construct() -> void override;
  auto destruct() -> void override;
  auto update(f64) -> void override;
  auto interpolate(f64) -> void override;
  auto interface() -> void override;
  auto handle_events(Event&) -> void override;
  auto on_resize(const Extent&) -> void override;
  auto render() -> void override;

private:
  GizmoState current_mode{ GizmoState::Translate };
  Scope<Renderer> renderer;
  Scope<Camera> camera;

  SceneState scene_state{ SceneState::Edit };
  Ref<Engine::Core::Scene> scene{ nullptr };
  glm::vec2 viewport_size{ 0, 0 };
  glm::vec2 viewport_position{ 0, 0 };
  Ref<entt::entity> selected_entity{ nullptr };
  auto perform_raycast(const glm::vec2&) -> entt::entity;
  auto handle_transform_mode(const KeyPressedEvent&) -> void;
  auto has_valid_entity() -> bool;

  using WidgetTuple =
    std::tuple<Scope<Widgets::SceneWidget>, Scope<Widgets::PerformanceWidget>>;
  WidgetTuple widgets;
};
