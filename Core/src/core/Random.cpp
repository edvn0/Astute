#include "pch/CorePCH.hpp"

#include "core/Random.hpp"

#include <random>

namespace Engine::Core::Random {

namespace {
std::random_device device{};
std::default_random_engine engine{ device() };
}

auto
random_in_rectangle(Core::i32 min, Core::i32 max) -> glm::vec3
{
  std::uniform_int_distribution distribution{ min, max };
  return {
    distribution(engine),
    distribution(engine),
    distribution(engine),
  };
}

auto
random_in(const AABB& aabb) -> glm::vec3
{
  auto x = random(aabb.min.x, aabb.max.x);
  auto y = random(aabb.min.y, aabb.max.y);
  auto z = random(aabb.min.z, aabb.max.z);
  return { x, y, z };
}

auto
random_colour() -> glm::vec4
{
  static std::uniform_real_distribution distribution{ 0.0, 1.0 };
  return {
    distribution(engine), distribution(engine), distribution(engine), 1.0F
  };
}

auto
random_colour3() -> glm::vec3
{
  static std::uniform_real_distribution distribution{ 0.0, 1.0 };
  return {
    distribution(engine),
    distribution(engine),
    distribution(engine),
  };
}

auto
random(Core::f32 min, Core::f32 max) -> Core::f32
{
  std::uniform_real_distribution distribution{ min, max };
  return distribution(engine);
}
auto
random(Core::f64 min, Core::f64 max) -> Core::f64
{
  std::uniform_real_distribution distribution{ min, max };
  return distribution(engine);
}
auto
random(Core::f64 min, Core::f32 max) -> Core::f64
{
  std::uniform_real_distribution distribution{ min,
                                               static_cast<Core::f64>(max) };
  return distribution(engine);
}
auto
random(Core::f32 min, Core::f64 max) -> Core::f64
{
  std::uniform_real_distribution distribution{ static_cast<Core::f64>(min),
                                               max };
  return distribution(engine);
}
auto
random_uint(Core::u64 min, Core::u64 max) -> Core::u64
{
  std::uniform_int_distribution distribution{ min, max };
  return distribution(engine);
}

} // namespace Engine::Core
