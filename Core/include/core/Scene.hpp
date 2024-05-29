#pragma once

#include "graphics/Forward.hpp"

#include "core/Camera.hpp"
#include "core/Random.hpp"
#include "core/Types.hpp"
#include "graphics/Material.hpp"
#include "graphics/Mesh.hpp"
#include "graphics/ShaderBuffers.hpp"

#include "thread_pool/ResultContainer.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <entt/entt.hpp>
#include <mutex>
#include <queue>
#include <utility>

namespace Engine::Core {

struct SimpleMeshComponent
{
  Ref<Graphics::VertexBuffer> vertex_buffer{ nullptr };
  Ref<Graphics::IndexBuffer> index_buffer{ nullptr };
  Ref<Graphics::Material> material{ nullptr };
  Ref<Graphics::Shader> shader{ nullptr };
};

struct MeshComponent
{
  Core::Ref<Graphics::StaticMesh> mesh;
};

struct TransformComponent
{
  glm::vec3 translation{ 0 };
  glm::quat rotation{ 1, 0, 0, 0 };
  glm::vec3 scale{ 1 };

  // Axis-Aligned Bounding Box in local space
  glm::vec3 aabb_min = glm::vec3(-0.5F);
  glm::vec3 aabb_max = glm::vec3(0.5F);

  [[nodiscard]] auto compute() const -> glm::mat4
  {
    static constexpr auto identity = glm::mat4(1);
    return glm::translate(identity, translation) * glm::mat4_cast(rotation) *
           glm::scale(identity, scale);
  }

  [[nodiscard]] auto intersects(const glm::vec3& ray,
                                const glm::vec3& origin) const -> bool
  {
    // Transform the ray and origin to local space
    glm::mat4 model_matrix = glm::translate(glm::mat4(1.0F), translation) *
                             glm::mat4_cast(rotation) *
                             glm::scale(glm::mat4(1.0F), scale);
    glm::mat4 inv_model_matrix = glm::inverse(model_matrix);

    glm::vec3 local_ray =
      glm::normalize(glm::vec3(inv_model_matrix * glm::vec4(ray, 0.0F)));
    glm::vec3 local_origin =
      glm::vec3(inv_model_matrix * glm::vec4(origin, 1.0F));

    float tmin = (aabb_min.x - local_origin.x) / local_ray.x;
    float tmax = (aabb_max.x - local_origin.x) / local_ray.x;

    if (tmin > tmax) {
      std::swap(tmin, tmax);
    }

    float tymin = (aabb_min.y - local_origin.y) / local_ray.y;
    float tymax = (aabb_max.y - local_origin.y) / local_ray.y;

    if (tymin > tymax) {
      std::swap(tymin, tymax);
    }

    if ((tmin > tymax) || (tymin > tmax)) {
      return false;
    }

    if (tymin > tmin) {
      tmin = tymin;
    }

    if (tymax < tmax) {
      tmax = tymax;
    }

    float tzmin = (aabb_min.z - local_origin.z) / local_ray.z;
    float tzmax = (aabb_max.z - local_origin.z) / local_ray.z;

    if (tzmin > tzmax) {
      std::swap(tzmin, tzmax);
    }

    if ((tmin > tzmax) || (tzmin > tmax)) {
      return false;
    }

    return true;
  }
};

struct IdentityComponent
{
  std::string name;
  Core::u64 unique_identifier;

  IdentityComponent(std::string name, Core::u64 unique_identifier)
    : name(std::move(name))
    , unique_identifier(unique_identifier)
  {
  }

  explicit IdentityComponent(std::string n)
    : name(std::move(n))
    , unique_identifier(Core::Random::random_uint(0))
  {
  }

  explicit IdentityComponent(const std::string_view n)
    : name(n)
    , unique_identifier(Core::Random::random_uint(0))
  {
  }
};

struct PointLightComponent
{
  glm::vec3 radiance{ 1.0F, 1.0F, 1.0F };
  float intensity{ 1.0F };
  float light_size{ 0.5F };
  float min_radius{ 1.f };
  float radius{ 10.0F };
  bool casts_shadows{ true };
  bool soft_shadows{ true };
  float falloff{ 1.0F };
};

struct SpotLightComponent
{
  glm::vec3 radiance{ 1.0F };
  float intensity{ 1.0F };
  float range{ 10.0F };
  float angle{ 60.0F };
  float angle_attenuation{ 5.0F };
  bool casts_shadows{ false };
  bool soft_shadows{ false };
  float falloff{ 1.0F };
};

struct LightEnvironment
{
  glm::vec4 sun_position{ 0 };
  glm::vec3 sun_direction{};
  glm::vec4 colour_and_intensity{ 0.2, 0.3, 0.1, 2.0 };
  glm::vec4 specular_colour_and_intensity{ 0.7, 0.2, 0.0, 3 };

  glm::mat4 shadow_projection{ 1 };
  bool is_perspective{ false };

  std::vector<Graphics::PointLight> point_lights;
  std::vector<Graphics::SpotLight> spot_lights;
};

class Scene
{
public:
  explicit Scene(std::string_view);

  auto on_update_editor(f64) -> void;
  auto on_render_editor(Graphics::Renderer&, const Camera&) -> void;

  auto set_name(std::string_view name_view) -> void { name = name_view; }
  auto clear() -> void { registry.clear(); }

  template<class... Ts>
  auto clear() -> void
  {
    registry.clear<Ts...>();
  }

  auto create_entity(std::string_view name) -> entt::entity;
  [[nodiscard]] auto get_light_environment() const -> const LightEnvironment&
  {
    return light_environment;
  }
  auto get_light_environment() -> LightEnvironment&
  {
    return light_environment;
  }

  template<class... Components>
  auto view()
  {
    return registry.view<Components...>();
  }
  auto find_intersected_entity(const glm::vec3&, const glm::vec3&)
    -> entt::entity;

private:
  std::mutex registry_mutex;
  entt::registry registry;

  std::string name;

  LightEnvironment light_environment;
  std::queue<std::future<void>> scene_tasks;
};

}
