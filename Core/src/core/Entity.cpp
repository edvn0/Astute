#include "pch/CorePCH.hpp"

#include "core/Entity.hpp"
#include "core/Scene.hpp"

namespace Engine::Core {

Entity::Entity(entt::registry* registry, entt::entity entity)
  : scene_registry(registry)
  , entity_handle(entity)
{
}

Entity::Entity(const Entity&) = default;

Entity::Entity(Entity&& other) noexcept
  : scene_registry(other.scene_registry)
  , entity_handle(other.entity_handle)
{
  other.scene_registry = nullptr;
  other.entity_handle = entt::null;
}

auto
Entity::operator=(const Entity& other) -> Entity&
{
  if (this != &other) {
    scene_registry = other.scene_registry;
    entity_handle = other.entity_handle;
  }
  return *this;
}

auto
Entity::operator=(Entity&& other) noexcept -> Entity&
{
  if (this != &other) {
    scene_registry = other.scene_registry;
    entity_handle = other.entity_handle;
    other.scene_registry = nullptr;
    other.entity_handle = entt::null;
  }
  return *this;
}

auto
Entity::get_aabb() -> AABB
{
  validate_component<MeshComponent>();
  auto copy = get<MeshComponent>().mesh->get_mesh_asset()->get_bounding_box();
  return copy;
}

auto
create_entity(entt::registry* registry, const std::string_view entity_name)
  -> Entity
{
  auto created_entity = registry->create();
  registry->emplace<IdentityComponent>(created_entity, entity_name);
  return { registry, created_entity };
}

auto
create_mesh_entity(entt::registry* registry,
                   const std::string_view path,
                   const glm::vec4& position) -> Entity
{
  auto created_entity = registry->create();
  registry->emplace<IdentityComponent>(created_entity, path);
  auto& transform = registry->emplace<TransformComponent>(created_entity);
  registry->emplace<MeshComponent>(created_entity,
                                   Graphics::StaticMesh::construct(path));
  transform.translation = position;
  return { registry, created_entity };
}

} // namespace Engine::Core
