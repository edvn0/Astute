#pragma once

#include "core/AABB.hpp"
#include "core/Types.hpp"

#include <cassert>
#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace Engine::Core {

class Entity
{
public:
  Entity(entt::registry* registry, entt::entity entity);
  Entity(const Entity&);
  Entity(Entity&& other) noexcept;

  auto operator=(const Entity& other) -> Entity&;
  auto operator=(Entity&& other) noexcept -> Entity&;

  template<typename T, typename... Args>
  auto emplace(Args&&... args) -> T&
  {
    validate_entity();
    return scene_registry->get_or_emplace<T>(entity_handle,
                                             std::forward<Args>(args)...);
  }

  auto get_aabb() -> AABB;

  template<typename T>
  auto get() -> T&
  {
    validate_entity();
    validate_component<T>();
    return scene_registry->get<T>(entity_handle);
  }

private:
  entt::registry* scene_registry{ nullptr };
  entt::entity entity_handle{ entt::null };

  auto validate_entity() -> void
  {
#ifdef ASTUTE_DEBUG
    assert(scene_registry != nullptr);
    assert(scene_registry->valid(entity_handle));
#endif
  }

  template<typename T>
  auto validate_component()
  {
#ifdef ASTUTE_DEBUG
    assert(scene_registry->any_of<T>());
#endif
  }
};

auto
create_entity(entt::registry* registry, std::string_view entity_name) -> Entity;
auto
create_mesh_entity(entt::registry* registry,
                   std::string_view path,
                   const glm::vec4& position) -> Entity;

} // namespace Engine::Core
