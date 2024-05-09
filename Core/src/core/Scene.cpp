#include "pch/CorePCH.hpp"

#include "core/Random.hpp"
#include "core/Scene.hpp"
#include "graphics/Material.hpp"
#include "graphics/Vertex.hpp"

#include "graphics/Renderer.hpp"

#include <ranges>

namespace Engine::Core {

Scene::Scene(const std::string_view name_view)
  : name(name_view)
{
  auto cube_mesh = Core::make_ref<Graphics::StaticMesh>(
    Core::make_ref<Graphics::MeshAsset>("Assets/meshes/cube/cube.gltf"));
  auto sponza_mesh =
    Graphics::StaticMesh::construct("Assets/meshes/sponza/sponza.obj");

  auto sponza = registry.create();
  registry.emplace<MeshComponent>(sponza, sponza_mesh);
  auto& transform2 = registry.emplace<TransformComponent>(sponza);
  transform2.translation = { 0, 5, 0 };
  transform2.rotation =
    glm::rotate(glm::mat4{ 1.0F }, glm::radians(180.0F), glm::vec3{ 1, 0, 0 });
  // Floor is big!
  transform2.scale = { 0.01, 0.01, 0.01 };

  // small cube on top with some offset
  auto entity3 = registry.create();
  registry.emplace<MeshComponent>(entity3, cube_mesh);
  auto& transform3 = registry.emplace<TransformComponent>(entity3);
  transform3.translation = { 0, 0, 0 };

  const auto& bounds = sponza_mesh->get_mesh_asset()->get_bounding_box();
  const auto scaled = bounds.scaled(0.01);
  for (auto i = 0; i < 300; i++) {
    auto light = registry.create();
    registry.emplace<MeshComponent>(light, cube_mesh);
    auto& t = registry.emplace<TransformComponent>(light);
    auto& light_data = registry.emplace<PointLightComponent>(light);
    t.scale *= 0.1;
    t.translation = Random::random_in(scaled);
    t.translation.y -= 5;
    light_data.radiance = Random::random_colour();
    light_data.intensity = Random::random(0.5, 1.0);
    light_data.light_size = Random::random(0.1, 1.0);
    light_data.min_radius = Random::random(1.0, 20.0);
    light_data.radius = Random::random(0.1, 30.0);
    light_data.falloff = Random::random(0.1, 0.5);
  }

  for (auto i = 0; i < 300; i++) {
    auto light = registry.create();
    auto& t = registry.emplace<TransformComponent>(light);
    registry.emplace<MeshComponent>(light, cube_mesh);
    t.scale *= 0.1;

    t.translation = Random::random_in(scaled);
    t.translation.y -= 5;
    auto& light_data = registry.emplace<SpotLightComponent>(light);
    light_data.radiance = Random::random_colour();
    light_data.angle = Random::random(30.0, 90.0);
    light_data.range = Random::random(0.1, 1.0);
    light_data.angle_attenuation = Random::random(1.0, 5.0);
    light_data.intensity = Random::random(0.5, 1.0);
  }
}

auto
Scene::on_update_editor(f64 ts) -> void
{
  static f64 rotation = glm::radians(60.0F);
  glm::vec3 begin{ 0 };
  static constexpr auto radius = 10.0F;
  begin.x = radius * glm::cos(rotation);
  begin.y = -radius;
  begin.z = radius * glm::sin(rotation);

  light_environment.sun_position = begin;

  // Improved gradient for color based on rotation
  // Using different frequencies and phases for color channels
  light_environment.colour_and_intensity = {
    0.5, 0.5, 0.5, 1
  }; // Base intensity

  light_environment.specular_colour_and_intensity = {
    0.1, 0.1, 0.1, 1
  }; // Base specular intensity
  light_environment.spot_lights.clear();
  light_environment.point_lights.clear();

  [&]() {
    static auto prev_count = 15U;
    auto count = 0U;

    light_environment.point_lights.reserve(prev_count + 1);

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

    if (count != prev_count) {
      prev_count = count;
    }
  }();

  [&]() {
    static auto prev_count = 15U;
    auto count = 0;
    light_environment.spot_lights.reserve(prev_count + 1);

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

    if (count != prev_count) {
      prev_count = count;
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
