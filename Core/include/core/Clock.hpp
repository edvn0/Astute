#pragma once

#include "core/Types.hpp"

namespace Engine::Core {

class Clock
{
public:
  static auto now() -> f64;
  static auto now_ms() -> f64;
};

} // namespace Engine::Core
