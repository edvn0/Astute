#pragma once

#include "AstuteCore.hpp"

using namespace Engine::Core;
using namespace Engine::Graphics;

class AstuteApplication : public Application
{
public:
  ~AstuteApplication() override;
  explicit AstuteApplication(Application::Configuration);

  auto update(f64 time_step) -> void override;
  auto interpolate(f64 superfluous_time_step) -> void override;
  auto interface() -> void override;
  auto handle_events(Event& event) -> void override;
  auto on_resize(const Extent& new_extent) -> void override;

private:
  enum class SceneState : u8
  {
    Edit = 0,
    Play = 1,
    Pause = 2,
    Simulate = 3
  };
  SceneState scene_state{ SceneState::Edit };
  Ref<Scene> scene{ nullptr };

  Renderer renderer;
  Camera camera;
};
