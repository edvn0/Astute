#include "graphics/Mesh.hpp"
#include "pch/CorePCH.hpp"

#include "core/StandardPaths.hpp"

#include "core/Entity.hpp"
#include "core/Maths.hpp"
#include "core/Random.hpp"
#include "core/Scene.hpp"
#include "graphics/Device.hpp"
#include "graphics/Material.hpp"
#include "graphics/TextureCube.hpp"
#include "graphics/Vertex.hpp"
#include "logging/Logger.hpp"

#include "graphics/Renderer.hpp"

#include <glm/gtc/random.hpp>
#include <limits>
#include <ranges>

namespace Engine::Core {

namespace Utilities {

auto
intersects(const Engine::Core::AABB& aabb,
           const glm::vec3& ray,
           const glm::vec3& origin) -> bool
{
  glm::vec3 inv_dir = 1.0F / ray;
  glm::vec3 t0s = (aabb.min - origin) * inv_dir;
  glm::vec3 t1s = (aabb.max - origin) * inv_dir;

  glm::vec3 tmins = glm::min(t0s, t1s);
  glm::vec3 tmaxs = glm::max(t0s, t1s);

  float tmin = glm::max(glm::max(tmins.x, tmins.y), tmins.z);
  float tmax = glm::min(glm::min(tmaxs.x, tmaxs.y), tmaxs.z);

  return tmax >= tmin;
}

auto
calculate_aabb(const TransformComponent& transform) -> Engine::Core::AABB
{
  glm::vec3 center = (transform.aabb_min + transform.aabb_max) * 0.5F;
  glm::vec3 extents = (transform.aabb_max - transform.aabb_min) * 0.5F;

  // Apply scale
  extents *= transform.scale;

  if (transform.rotation == glm::quat(1, 0, 0, 0)) {
    // No rotation, simply offset the center and adjust extents
    center = transform.translation +
             glm::vec3(transform.compute() * glm::vec4(center, 1.0F));
    return { center - extents, center + extents };
  } // With rotation, calculate new AABB
  glm::mat3 rotation_matrix = glm::mat3_cast(transform.rotation);
  glm::vec3 new_extents(0);

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      new_extents[i] += std::abs(rotation_matrix[i][j] * extents[j]);
    }
  }

  glm::vec3 new_center =
    transform.translation +
    glm::vec3(transform.compute() * glm::vec4(center, 1.0F));
  return { new_center - new_extents, new_center + new_extents };
}

}

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
    Core::make_ref<Graphics::MeshAsset>(meshes_file("cube/cube.gltf")));

  std::optional<AABB> mesh_aabb;
  {
    create_mesh_entity(meshes_file("formula1/formula1.gltf"), { 0, 0, 0, 0 });
  }
  {
    auto entity =
      create_mesh_entity(meshes_file("sponza_new/sponza.gltf"), { 0, 0, 0, 0 });
    entity.get<TransformComponent>().scale *= 0.01;
    entity.get<TransformComponent>().rotation =
      glm::rotate(glm::radians(180.0F), glm::vec3{ 1, 0, 0 });
    entity.get<TransformComponent>().translation.y += 7.0F;
    mesh_aabb = entity.get_aabb();
    mesh_aabb->scale_to(0.01);
    mesh_aabb->rotate(glm::rotate(glm::radians(180.0F), glm::vec3{ 1, 0, 0 }));
  }

  {
    auto floor = create_entity("Floor");
    floor.emplace<MeshComponent>(cube_mesh);
    auto& floor_transform = floor.emplace<TransformComponent>();
    floor_transform.translation.y += 2;
    floor_transform.scale = { 100, 1, 100 };
  }

  const auto& bounds = *mesh_aabb;
  const auto& scaled = bounds.scaled(1.0);

  auto move_light = [s = scaled](Entity& entity, Core::f64 ts) {
    auto& transform = entity.get<TransformComponent>();
    auto& velocity = entity.get<VelocityComponent>();
    auto& direction = velocity.direction;
    auto& speed = velocity.speed;

    transform.translation += direction * speed * static_cast<float>(ts);

    if (transform.translation.x >= s.max.x) {
      direction.x *= -1;
    }
    if (transform.translation.x <= s.min.x) {
      direction.x *= -1;
    }
    if (transform.translation.y >= s.max.y) {
      direction.y *= -1;
    }
    if (transform.translation.y <= s.min.y) {
      direction.y *= -1;
    }
    if (transform.translation.z >= s.max.z) {
      direction.z *= -1;
    }
    if (transform.translation.z <= s.min.z) {
      direction.z *= -1;
    }
  };

  for (auto i = 0; i < 127; i++) {
    auto light = create_entity(std::format("PointLight{}", i));
    light.emplace<MeshComponent>(cube_mesh);
    auto& t = light.emplace<TransformComponent>();
    light.emplace<ScriptComponent>(move_light);
    glm::vec3 random_direction = glm::sphericalRand(1.0F);
    light.emplace<VelocityComponent>(random_direction);
    t.scale *= 0.1;
    auto& light_data = light.emplace<PointLightComponent>();
    t.translation = Random::random_in(scaled);
    light_data.radiance = Random::random_colour();
    light_data.intensity = Random::random<Core::f32>(0.5, 1.0);
    light_data.light_size = Random::random<Core::f32>(0.1, 1.0);
    light_data.min_radius = Random::random<Core::f32>(1.0, 20.0);
    light_data.radius = Random::random<Core::f32>(0.1, 30.0);
    light_data.falloff = Random::random<Core::f32>(0.1, 10.0);
  }

  for (auto i = 0; i < 127; i++) {
    auto light = create_entity(std::format("SpotLight{}", i));
    auto& t = light.emplace<TransformComponent>();
    light.emplace<MeshComponent>(cube_mesh);
    light.emplace<ScriptComponent>(move_light);
    glm::vec3 random_direction = glm::sphericalRand(1.0F);
    light.emplace<VelocityComponent>(random_direction);
    t.scale *= 0.1;

    t.translation = Random::random_in(scaled);
    auto& light_data = light.emplace<SpotLightComponent>();
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
  static f64 time = 0.0F;
  time += ts;

  // Update sun position to orbit around the origin
  static constexpr auto sun_radius = sqrt(-30 * 30 + -70 * -70 + 30 * 30);
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

  for (auto&& [entt_handle, script] : registry.view<ScriptComponent>().each()) {
    if (!script.is_valid()) {
      continue;
    }
    Entity entity{ &registry, entt_handle };
    script.on_update(entity, ts);
  }
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
    Engine::Core::AABB aabb = Utilities::calculate_aabb(transform);
    renderer.get_2d_renderer().submit_aabb(
      aabb, transform.compute(), { 0.1, 0.9, 0.8, 1.0 });
  }

  for (auto&& [entity, light, mesh, transform] :
       registry.view<PointLightComponent, MeshComponent, TransformComponent>()
         .each()) {
    const auto light_colour = light.radiance * light.intensity;
    renderer.submit_static_light(
      mesh.mesh, transform.compute(), glm::vec4{ light_colour, 1.0F });
  }

  for (auto&& [entity, light, mesh, transform] :
       registry.view<SpotLightComponent, MeshComponent, TransformComponent>()
         .each()) {
    const auto light_colour = light.radiance * light.intensity;
    renderer.submit_static_light(
      mesh.mesh, transform.compute(), glm::vec4{ light_colour, 1.0F });
  }

  renderer.end_scene();
}

auto
Scene::create_entity(const std::string_view entity_name) -> Entity
{
  return Engine::Core::create_entity(&registry, entity_name);
}

auto
Scene::create_mesh_entity(const std::string_view path,
                          const glm::vec4& position) -> Entity
{
  return Engine::Core::create_mesh_entity(&registry, path, position);
}

auto
Scene::create_mesh_entity(const std::filesystem::path& path,
                          const glm::vec4& position) -> Entity
{
  return Engine::Core::create_mesh_entity(&registry, path.string(), position);
}

auto
Scene::find_intersected_entity(const glm::vec3& ray,
                               const glm::vec3& camera_position) -> entt::entity
{
  auto closest_distance = std::numeric_limits<float>::max();
  entt::entity closest_entity = entt::null;

  auto view = registry.view<const TransformComponent>(
    entt::exclude<PointLightComponent, SpotLightComponent>);
  for (auto&& [entity, transform] : view.each()) {
    const auto aabb = Utilities::calculate_aabb(transform);
    if (Utilities::intersects(aabb, ray, camera_position)) {
      float distance = glm::distance(camera_position, transform.translation);
      if (distance < closest_distance) {
        closest_distance = distance;
        closest_entity = entity;
      }
    }
  }

  return closest_entity;
}

} // namespace Engine::Core
