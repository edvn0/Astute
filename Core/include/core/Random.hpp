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

auto
random(Core::f32 min, Core::f32 max) -> Core::f32;
auto
random(Core::f64 min, Core::f64 max) -> Core::f64;
auto
random(Core::f64 min, Core::f32 max) -> Core::f64;
auto
random(Core::f32 min, Core::f64 max) -> Core::f64;

inline auto
random() -> Core::f64
{
  return random(0.0, 1.0);
}

auto
random(std::floating_point auto min, std::floating_point auto max)
  -> std::remove_cvref_t<decltype(min)>
{
  return random(min, max);
}

} // namespace Engine::Core
