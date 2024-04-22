#pragma once

#include "graphics/Forward.hpp"

#include "core/Camera.hpp"
#include "core/Types.hpp"
#include "graphics/Material.hpp"
#include "graphics/ShaderBuffers.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <entt/entt.hpp>

namespace Engine::Core {

struct SimpleMeshComponent
{
  Ref<Graphics::VertexBuffer> vertex_buffer{ nullptr };
  Ref<Graphics::IndexBuffer> index_buffer{ nullptr };
  Ref<Graphics::Material> material{ nullptr };
  Ref<Graphics::Shader> shader{ nullptr };
};

struct TransformComponent
{
  glm::vec3 translation{ 0 };
  glm::quat rotation{ 1, 0, 0, 0 };
  glm::vec3 scale{ 1 };

  auto compute() -> glm::mat4
  {
    return glm::translate(glm::mat4(1), translation) *
           glm::mat4_cast(rotation) * glm::scale(glm::mat4(1), scale);
  }
};

struct PointLightComponent
{
  glm::vec3 radiance{ 1.0f, 1.0f, 1.0f };
  float intensity{ 1.0f };
  float light_size{ 0.5f };
  float min_radius{ 1.f };
  float radius{ 10.0f };
  bool casts_shadows{ true };
  bool soft_shadows{ true };
  float falloff{ 1.0f };
};

struct SpotLightComponent
{
  glm::vec3 radiance{ 1.0f };
  float intensity{ 1.0f };
  float range{ 10.0f };
  float angle{ 60.0f };
  float angle_attenuation{ 5.0f };
  bool casts_shadows{ false };
  bool soft_shadows{ false };
  float falloff{ 1.0f };
};

struct LightEnvironment
{
  glm::vec3 sun_position{ 0 };
  glm::vec4 colour_and_intensity{ 0 };
  glm::vec4 specular_colour_and_intensity{ 0 };

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

  auto get_light_environment() const -> const LightEnvironment&
  {
    return light_environment;
  }

private:
  entt::registry registry;
  std::string name;

  LightEnvironment light_environment;
};
}
