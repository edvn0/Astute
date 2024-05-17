#pragma once

#include <glm/glm.hpp>

#include <type_traits>

namespace Engine::Core {

struct AABB
{
  glm::vec3 min, max;

  constexpr AABB()
    : min(0.0f)
    , max(0.0f)
  {
  }

  constexpr AABB(const glm::vec3& in_min, const glm::vec3& in_max)
    : min(in_min)
    , max(in_max)
  {
  }

  auto update_min_max(const glm::vec3& new_position) -> void
  {
    min.x = glm::min(new_position.x, min.x);
    min.y = glm::min(new_position.y, min.y);
    min.z = glm::min(new_position.z, min.z);

    max.x = glm::max(new_position.x, max.x);
    max.y = glm::max(new_position.y, max.y);
    max.z = glm::max(new_position.z, max.z);
  }

  auto scaled(std::floating_point auto value) const -> Core::AABB
  {
    return AABB(min * static_cast<float>(value),
                max * static_cast<float>(value));
  }
};

} // namespace Engine::Core
