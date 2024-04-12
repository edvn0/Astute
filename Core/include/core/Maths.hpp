#pragma once

#include "core/Types.hpp"

#include <array>

namespace Engine::Core {

template<std::floating_point T, Core::usize N>
  requires(N > 0 && N <= 4)
struct Vector
{
  std::array<T, N> data{};

  Vector(T x, T y, T z, T w)
  {
    if constexpr (N == 4) {
      if (x > 1 || y > 1 || z > 1 || w > 1) {
        data = { x / 255.0F, y / 255.0F, z / 255.0F, w / 255.0F };
      } else {
        data = { x, y, z, w };
      }
    }
  }

  explicit Vector(T x)
  {
    data.at(0) = x;
    data.at(1) = x;
    data.at(2) = x;
    data.at(3) = x;
  }
};

using Vec4 = Vector<Core::f32, 4>;

} // namespace Engine::Core
