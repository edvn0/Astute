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
random_colour() -> glm::vec4
{
  static std::uniform_real_distribution distribution{ 0.0, 1.0 };
  return {
    distribution(engine), distribution(engine), distribution(engine), 1.0F
  };
}

} // namespace Engine::Core
