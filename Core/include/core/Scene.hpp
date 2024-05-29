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

  auto compute() const -> glm::mat4
  {
    static constexpr auto identity = glm::mat4(1);
    return glm::translate(identity, translation) * glm::mat4_cast(rotation) *
           glm::scale(identity, scale);
  }
};

struct IdentityComponent
{
  std::string name;
  Core::u64 unique_identifier;

  IdentityComponent(const std::string& name, Core::u64 unique_identifier)
    : name(name)
    , unique_identifier(unique_identifier)
  {
  }

  IdentityComponent(const std::string& n)
    : name(n)
    , unique_identifier(Core::Random::random_uint(0))
  {
  }

  IdentityComponent(const std::string_view n)
    : name(n)
    , unique_identifier(Core::Random::random_uint(0))
  {
  }
};

struct PointLightComponent
{
  glm::vec3 radiance{ 1.0F, 1.0F, 1.0F };
  float intensity{ 1.0F };
  float light_size{ 0.5f };
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

  auto get_light_environment() const -> const LightEnvironment&
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

private:
  std::mutex registry_mutex;
  entt::registry registry;

  std::string name;

  LightEnvironment light_environment;
  std::queue<std::future<void>> scene_tasks;
};

}
