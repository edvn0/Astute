#pragma once

#include "core/Types.hpp"

#include "core/Camera.hpp"
#include "graphics/Renderer.hpp"

namespace Engine::Core {

class Scene
{
public:
  auto on_update_editor(f64) -> void;
  auto on_render_editor(Graphics::Renderer&, f64, const Camera&) -> void;
};

}