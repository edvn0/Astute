#include "pch/CorePCH.hpp"

#include "core/Scene.hpp"

#include "graphics/Renderer.hpp"

namespace Engine::Core {

Scene::Scene(const std::string_view name_view)
  : name(name_view)
{
}

auto
Scene::on_update_editor(f64 ts) -> void
{
}

auto
Scene::on_render_editor(Graphics::Renderer& renderer,
                        f64 ts,
                        const Camera& camera) -> void
{
  renderer.begin_scene(*this,
                       {
                         camera,
                         camera.get_view_matrix(),
                         camera.get_near_plane(),
                         camera.get_far_plane(),
                         camera.get_fov(),
                       });

  // TODO: Submit draw lists

  renderer.end_scene();
}

} // namespace Engine::Core
