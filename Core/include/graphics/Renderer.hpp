#pragma once

#include "core/Forward.hpp"

#include "core/Types.hpp"

#include "graphics/Framebuffer.hpp"
#include "graphics/GraphicsPipeline.hpp"
#include "graphics/Material.hpp"

namespace Engine::Graphics {

class Renderer
{
public:
  auto begin_scene(Core::Scene&) -> void;
  auto end_scene() -> void;

private:
  Core::Scope<Framebuffer> geometry_framebuffer{ nullptr };
  Core::Scope<GraphicsPipeline> geometry_pipeline{ nullptr };
  Core::Scope<Material> geometry_material{ nullptr };
};

}
