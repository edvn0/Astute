#pragma once

#include "core/AABB.hpp"
#include "core/Types.hpp"

#include <glm/glm.hpp>

#include <concepts>
#include <limits>
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
random_in(const AABB&) -> glm::vec3;

auto
random_colour() -> glm::vec4;
auto
random_colour3() -> glm::vec3;

auto
random(Core::f32 min, Core::f32 max) -> Core::f32;
auto
random(Core::f64 min, Core::f64 max) -> Core::f64;
auto
random(Core::f64 min, Core::f32 max) -> Core::f64;
auto
random(Core::f32 min, Core::f64 max) -> Core::f64;
auto
random_uint(Core::u64 min = 0,
            Core::u64 max = std::numeric_limits<Core::u64>::max()) -> Core::u64;

inline auto
random() -> Core::f64
{
  return random(0.0, 1.0);
}

template<std::floating_point Out = Core::f32>
auto
random(std::floating_point auto min, std::floating_point auto max) -> Out
{
  return static_cast<Out>(random(min, max));
}

} // namespace Engine::Core
