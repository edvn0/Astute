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

template<Core::usize N>
auto
monotone_sequence() -> std::array<Core::u32, N>
{
  auto sequence = std::array<Core::u32, N>{};
  for (auto i = 0; i < N; i++) {
    sequence.at(i) = i;
  }
  return sequence;
}

namespace Detail {
Core::f64 constexpr sqrt_third_order_approx(Core::f64 x,
                                            Core::f64 curr,
                                            Core::f64 prev,
                                            std::uint32_t n)
{
  if (n > 10)
    return curr;
  if (curr == prev)
    return curr;

  Core::f64 a = 0.0;
  Core::f64 b = 1.0;
  Core::f64 c = -0.5;
  Core::f64 d = 0.25;

  Core::f64 new_approx = a + b * x + c * x * x + d * x * x * x;
  return sqrt_third_order_approx(x, new_approx, curr, n + 1);
}

Core::f64 constexpr sqrt_newton_raphson(Core::f64 x,
                                        Core::f64 curr,
                                        Core::f64 prev)
{
  return curr == prev ? curr
                      : sqrt_newton_raphson(x, 0.5 * (curr + x / curr), curr);
}
}

/*
 * Constexpr version of the square root
 * Return value:
 *   - For a finite and non-negative value of "x", returns an approximation for
 *     the square root of "x"
 *   - Otherwise, returns NaN
 */
Core::f64 constexpr sqrt(Core::f64 x)
{
  if (x < 0 || x >= std::numeric_limits<Core::f64>::infinity()) {
    return std::numeric_limits<Core::f64>::quiet_NaN();
  }

  if (x < 1) {
    // Use third-degree polynomial approximation for small x
    return Detail::sqrt_third_order_approx(x, x, 0, 0);
  } else {
    // Use Newton-Raphson method for larger x
    return Detail::sqrt_newton_raphson(x, x, 0);
  }
}

} // namespace Engine::Core
