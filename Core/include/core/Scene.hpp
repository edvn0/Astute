#pragma once

#include "graphics/Forward.hpp"

#include "core/Camera.hpp"
#include "core/Types.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <entt/entt.hpp>

namespace Engine::Core {

struct SimpleMeshComponent
{
  Scope<Graphics::VertexBuffer> vertex_buffer{ nullptr };
  Scope<Graphics::IndexBuffer> index_buffer{ nullptr };
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

struct LightEnvironment
{
  glm::vec3 position{ 0 };
  glm::vec4 colour_and_intensity{ 0 };
  glm::vec4 specular_colour_and_intensity{ 0 };
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