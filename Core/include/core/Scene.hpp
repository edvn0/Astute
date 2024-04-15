#pragma once

#include "graphics/Forward.hpp"

#include "core/Camera.hpp"
#include "core/Types.hpp"

#include <entt/entt.hpp>

namespace Engine::Core {

class Scene
{
public:
  explicit Scene(std::string_view);

  auto on_update_editor(f64) -> void;
  auto on_render_editor(Graphics::Renderer&, f64, const Camera&) -> void;

  auto set_name(std::string_view name_view) -> void { name = name_view; }
  auto clear() -> void { registry.clear(); }

  template<class... Ts>
  auto clear() -> void
  {
    registry.clear<Ts...>();
  }

private:
  entt::registry registry;
  std::string name;
};

}