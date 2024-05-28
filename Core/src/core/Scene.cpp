#include "pch/CorePCH.hpp"

#include "core/Maths.hpp"
#include "core/Random.hpp"
#include "core/Scene.hpp"
#include "graphics/Device.hpp"
#include "graphics/Material.hpp"
#include "graphics/Vertex.hpp"
#include "logging/Logger.hpp"

#include "graphics/Renderer.hpp"

#include <limits>
#include <ranges>

namespace Engine::Core {

static constexpr auto sun_radius = sqrt(30 * 30 + 70 * 70 + 30 * 30);

template<typename Component, typename Light>
void
map(const glm::vec3& position,
    const Component& component,
    Light& light) = delete;

template<>
void
map(const glm::vec3& position,
    const PointLightComponent& component,
    Graphics::PointLight& light)
{
  light.pos = position;
  light.radiance = component.radiance;
  light.intensity = component.intensity;
  light.light_size = component.light_size;
  light.min_radius = component.min_radius;
  light.radius = component.radius;
  light.falloff = component.falloff;
}

template<>
void
map(const glm::vec3& position,
    const SpotLightComponent& component,
    Graphics::SpotLight& light)
{
  light.pos = position;
  light.radiance = component.radiance;
  light.intensity = component.intensity;
  light.range = component.range;
  light.angle = component.angle;
  light.angle_attenuation = component.angle_attenuation;
  light.casts_shadows = component.casts_shadows;
  light.soft_shadows = component.soft_shadows;
  light.falloff = component.falloff;
}

template<typename Component, typename Light, typename Container>
void
update_lights(entt::registry& registry,
              Container& lights,
              std::size_t& prev_count)
{
  auto count = 0U;
  lights.reserve(prev_count + 1);

  for (auto&& [entity, transform, light_component] :
       registry.view<TransformComponent, Component>().each()) {
    auto& light = lights.emplace_back();
    map(transform.translation, light_component, light);
    count++;
  }

  if (count != prev_count) {
    prev_count = count;
  }
}

Scene::Scene(const std::string_view name_view)
  : name(name_view)
{
  auto cube_mesh = Core::make_ref<Graphics::StaticMesh>(
    Core::make_ref<Graphics::MeshAsset>("Assets/meshes/cube/cube.gltf"));

  info("Creating scene: {}", name);
  auto sponza_mesh =
    Graphics::StaticMesh::construct("Assets/meshes/sponza_new/sponza.gltf");
  auto cerberus_mesh =
    Graphics::StaticMesh::construct("Assets/meshes/cerb/cerberus.gltf");

  {
    auto sponza = registry.create();
    registry.emplace<MeshComponent>(sponza, sponza_mesh);
    auto& transform2 = registry.emplace<TransformComponent>(sponza);
    transform2.translation = { 0, 5, 0 };
    transform2.rotation = glm::rotate(
      glm::mat4{ 1.0F }, glm::radians(180.0F), glm::vec3{ 1, 0, 0 });
    // Floor is big!
    transform2.scale = { 0.01, 0.01, 0.01 };
  }

  {
    auto cerberos = registry.create();
    registry.emplace<MeshComponent>(cerberos, cerberus_mesh);
    auto& transform2 = registry.emplace<TransformComponent>(cerberos);
    transform2.translation = { 0, 5, 0 };
    transform2.rotation = glm::rotate(
      glm::mat4{ 1.0F }, glm::radians(180.0F), glm::vec3{ 1, 0, 0 });
    // Floor is big!
    transform2.scale = { 0.01, 0.01, 0.01 };
  }

  const auto& bounds = sponza_mesh->get_mesh_asset()->get_bounding_box();
  const auto scaled = bounds.scaled(0.01);
  for (auto i = 0; i < 127; i++) {
    auto light = registry.create();
    registry.emplace<MeshComponent>(light, cube_mesh);
    auto& t = registry.emplace<TransformComponent>(light);
    auto& light_data = registry.emplace<PointLightComponent>(light);
    t.scale *= 0.1;
    t.translation = Random::random_in(scaled);
    t.translation.y -= 5;
    light_data.radiance = Random::random_colour();
    light_data.intensity = Random::random<Core::f32>(0.5, 1.0);
    light_data.light_size = Random::random<Core::f32>(0.1, 1.0);
    light_data.min_radius = Random::random<Core::f32>(1.0, 20.0);
    light_data.radius = Random::random<Core::f32>(0.1, 30.0);
    light_data.falloff = Random::random<Core::f32>(0.1, 10.0);
  }

  for (auto i = 0; i < 127; i++) {
    auto light = registry.create();
    auto& t = registry.emplace<TransformComponent>(light);
    registry.emplace<MeshComponent>(light, cube_mesh);
    t.scale *= 0.1;

    t.translation = Random::random_in(scaled);
    t.translation.y -= 5;
    auto& light_data = registry.emplace<SpotLightComponent>(light);
    light_data.radiance = Random::random_colour();
    light_data.angle = Random::random<Core::f32>(30.0, 90.0);
    light_data.range = Random::random<Core::f32>(0.1, 1.0);
    light_data.angle_attenuation = Random::random<Core::f32>(1.0, 5.0);
    light_data.intensity = Random::random<Core::f32>(0.5, 10.0);
  }
  light_environment.sun_position = { -30.0, -70.0, 30.0, 1.0 };
}

auto
Scene::on_update_editor(f64 ts) -> void
{
  static f64 time = 0.0f;
  time += ts;

  // Update sun position to orbit around the origin
  light_environment.sun_position = glm::vec4{
    sun_radius * glm::cos(time * 0.1F),
    -70.0F,
    sun_radius * glm::sin(time * 0.1F),
    1.0F,
  };

  if (!scene_tasks.empty()) {
    auto& task = scene_tasks.front();
    if (task.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
      task.get();
      scene_tasks.pop();
    }
  }

  light_environment.spot_lights.clear();
  light_environment.point_lights.clear();

  static auto prev_point_light_count = 15ULL;
  static auto prev_spot_light_count = 15ULL;
  update_lights<PointLightComponent, Graphics::PointLight>(
    registry, light_environment.point_lights, prev_point_light_count);
  update_lights<SpotLightComponent, Graphics::SpotLight>(
    registry, light_environment.spot_lights, prev_spot_light_count);
}

auto
Scene::on_render_editor(Graphics::Renderer& renderer,
                        const Camera& camera) -> void
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
