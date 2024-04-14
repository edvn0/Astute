#include "pch/CorePCH.hpp"

#include "core/Scene.hpp"

namespace Engine::Core
{

auto Scene::on_update_editor(f64 ts) -> void
{
}

auto Scene::on_render_editor(Graphics::Renderer& renderer, f64 ts, const Camera& camera) -> void
{
    renderer.begin_scene(*this);
    renderer.end_scene();
}
    
} // namespace Engine::Core

