#include "pch/CorePCH.hpp"

#include "core/Random.hpp"
#include "core/Scene.hpp"
#include "graphics/Material.hpp"
#include "graphics/Vertex.hpp"

#include "graphics/Renderer.hpp"

namespace Engine::Core {

Scene::Scene(const std::string_view name_view)
  : name(name_view)
{

  auto entity = registry.create();
  auto& [mesh] = registry.emplace<MeshComponent>(entity);
  mesh = Core::make_ref<Graphics::StaticMesh>(
    Core::make_ref<Graphics::MeshAsset>("Assets/meshes/cerb/scene.gltf"));

  auto& transform = registry.emplace<TransformComponent>(entity);
  transform.translation = { 0, 0, 0 };

  auto cube_mesh = Core::make_ref<Graphics::StaticMesh>(
    Core::make_ref<Graphics::MeshAsset>("Assets/meshes/cube.obj"));
  auto entity2 = registry.create();
  registry.emplace<MeshComponent>(entity2, cube_mesh);
  auto& transform2 = registry.emplace<TransformComponent>(entity2);
  transform2.translation = { 0, 5, 0 };
  // Floor is big!
  transform2.scale = { 10, 1, 10 };

  for (auto i = 0; i < 100; i++) {
    auto light = registry.create();
    registry.emplace<MeshComponent>(light, cube_mesh);
    auto& t = registry.emplace<TransformComponent>(light);
    auto& light_data = registry.emplace<PointLightComponent>(light);
    t.scale *= 0.1;
    t.translation = Random::random_in_rectangle(-50, 50);
    t.translation.y *= 3;
    light_data.radiance = Random::random_colour();
  }

  for (auto i = 0; i < 100; i++) {
    auto light = registry.create();
    auto& t = registry.emplace<TransformComponent>(light);
    registry.emplace<MeshComponent>(light, cube_mesh);
    t.scale *= 0.1;

    t.translation = Random::random_in_rectangle(-50, 50);

    auto& light_data = registry.emplace<SpotLightComponent>(light);
    light_data.radiance = Random::random_colour();
  }
}

auto
Scene::on_update_editor(f64 ts) -> void
{
  static f64 rotation = 0;
  rotation += 0.1 * ts;
  glm::vec3 begin{ 0 };
  begin.x = 10 * glm::cos(rotation);
  begin.y = -10;
  begin.z = 10 * glm::sin(rotation);

  light_environment.sun_position = begin;

  light_environment.colour_and_intensity = { 0.6, 0.1, 0.7, 1 };
  light_environment.specular_colour_and_intensity = { 0.2, 0.9, 0, 1 };

  light_environment.spot_lights.clear();
  light_environment.point_lights.clear();

  [&]() {
    auto count = 0U;

    for (auto&& [entity, transform, point_light] :
         registry.view<TransformComponent, PointLightComponent>().each()) {
      auto& light = light_environment.point_lights.emplace_back();
      light.pos = transform.translation;
      light.casts_shadows = point_light.casts_shadows;
      light.falloff = point_light.falloff;
      light.intensity = point_light.intensity;
      light.light_size = point_light.light_size;
      light.min_radius = point_light.min_radius;
      light.radiance = point_light.radiance;
      light.radius = point_light.radius;
      count++;
    }
  }();

  [&]() {
    auto count = 0;
    for (auto&& [entity, transform, spot_light] :
         registry.view<TransformComponent, SpotLightComponent>().each()) {
      auto& light = light_environment.spot_lights.emplace_back();
      light.pos = transform.translation;
      light.radiance = spot_light.radiance;
      light.intensity = spot_light.intensity;
      light.range = spot_light.range;
      light.angle = spot_light.angle;
      light.angle_attenuation = spot_light.angle_attenuation;
      light.casts_shadows = spot_light.casts_shadows;
      light.soft_shadows = spot_light.soft_shadows;
      light.falloff = spot_light.falloff;
      count++;
    }
  }();
}

auto
Scene::on_render_editor(Graphics::Renderer& renderer, const Camera& camera)
  -> void
{
  renderer.begin_scene(*this,
                       {
                         camera,
                         camera.get_view_matrix(),
                         camera.get_near_clip(),
                         camera.get_far_clip(),
                         camera.get_fov(),
                       });
  for (auto&& [entity, mesh, transform] :
       registry
         .view<MeshComponent, TransformComponent>(
           entt::exclude<PointLightComponent, SpotLightComponent>)
         .each()) {
    renderer.submit_static_mesh(mesh.mesh, transform.compute());
  }

  for (auto&& [entity, light, mesh, transform] :
       registry.view<PointLightComponent, MeshComponent, TransformComponent>()
         .each()) {
    renderer.submit_static_light(mesh.mesh, transform.compute());
  }

  for (auto&& [entity, light, mesh, transform] :
       registry.view<SpotLightComponent, MeshComponent, TransformComponent>()
         .each()) {
    renderer.submit_static_light(mesh.mesh, transform.compute());
  }

  renderer.end_scene();
}

} // namespace Engine::Core
