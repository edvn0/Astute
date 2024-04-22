#pragma once

#include "core/Types.hpp"

#include <glm/glm.hpp>

#include <concepts>
#include <type_traits>

namespace Engine::Core::Random {

auto random_in_rectangle(Core::i32, Core::i32) -> glm::vec3;

auto
random_in_rectangle(std::integral auto min, std::integral auto max) -> glm::vec3
{
  return random_in_rectangle(static_cast<Core::i32>(min),
                             static_cast<Core::i32>(max));
}

auto
random_colour() -> glm::vec4;

} // namespace Engine::Core
