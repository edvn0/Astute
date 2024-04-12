#pragma once

#include "core/Types.hpp"

#include <array>

namespace Engine::Core {

template<std::floating_point T, Core::usize N>
  requires(N >= 2 && N <= 4)
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
    } else if constexpr (N == 3) {
      if (x > 1 || y > 1 || z > 1) {
        data = { x / 255.0F, y / 255.0F, z / 255.0F, 1.0F };
      } else {
        data = { x, y, z, 1.0F };
      }
    } else if constexpr (N == 2) {
      if (x > 1 || y > 1) {
        data = { x / 255.0F, y / 255.0F, 1.0F, 1.0F };
      } else {
        data = { x, y, 1.0F, 1.0F };
      }
    }
  }

  Vector(T x, T y)
  {
    static_assert(N == 2);
    data.at(0) = x;
    data.at(1) = y;
  }

  explicit Vector(T x)
  {
    for (auto i = 0; i < N; i++) {
      data.at(i) = x;
    }
  }
};

using Vec4 = Vector<Core::f32, 4>;
using Vec3 = Vector<Core::f32, 3>;
using Vec2 = Vector<Core::f32, 2>;

} // namespace Engine::Core
