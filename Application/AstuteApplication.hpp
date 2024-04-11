#pragma once

#include "AstuteCore.hpp"

using namespace Engine::Core;
using namespace Engine::Graphics;

class AstuteApplication : public Application
{
public:
  ~AstuteApplication() override;
  explicit AstuteApplication(Application::Configuration);

  auto update(f64) -> void override;
  auto interpolate(f64) -> void override;
  auto interface() -> void override;
  auto handle_events(Event& event) -> void override;
};
