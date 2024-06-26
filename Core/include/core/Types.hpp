#pragma once

#include "core/DebugBreak.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>

#include "core/Exceptions.hpp"

namespace Engine::Core {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using byte = std::byte;
using usize = std::size_t;

using f32 = float;
using f64 = double;

template<class T>
concept Number = std::is_integral_v<T> || std::is_floating_point_v<T>;

template<Number T>
struct BasicExtent
{
  T width{};
  T height{};

  BasicExtent() = default;

  explicit BasicExtent(T val)
    : width(val)
    , height(val)
  {
  }

  BasicExtent(T w, T h)
    : width(w)
    , height(h)
  {
  }

  // Constructor for integral types separate from T
  template<typename U = T>
    requires(std::is_integral_v<U> && !std::is_same_v<T, U>)
  BasicExtent(U w, U h)
    : width(static_cast<T>(w))
    , height(static_cast<T>(h))
  {
  }

  template<class U>
    requires(!std::is_same_v<T, U>)
  auto as() const -> BasicExtent<U>
  {
    return {
      static_cast<U>(width),
      static_cast<U>(height),
    };
  }

  auto aspect_ratio() const -> f32
  {
    return static_cast<f32>(width) / static_cast<f32>(height);
  }

  auto operator==(const BasicExtent& other) const -> bool
  {
    return width == other.width && height == other.height;
  }

  template<Number U>
  auto operator==(const BasicExtent<U>& other) const -> bool
  {
    const auto casted = other.template as<T>();
    return width == casted.width && height == casted.height;
  }

  auto operator*(std::floating_point auto scale) const -> Core::BasicExtent<T>
  {
    return {
      static_cast<T>(scale * width),
      static_cast<T>(scale * height),
    };
  }

  auto valid() const { return width != T{ 0 } || height != T{ 0 }; }
};

using Extent = BasicExtent<u32>;
using FloatExtent = BasicExtent<f32>;

namespace Detail {
template<class T>
concept Deleter = requires(T t, void* data) {
  { t.operator()(data) } -> std::same_as<void>;
};

struct DefaultDelete
{
  template<class T>
  auto operator()(T* ptr) const noexcept -> void
  {
    if constexpr (requires(T* p) { p->destroy(); }) {
      ptr->destroy();
      delete ptr;
    } else {
      delete ptr;
    }
  }
};
}

template<class T, Detail::Deleter D = Detail::DefaultDelete>
using Scope = std::unique_ptr<T, D>;

template<class T, typename... Args>
[[nodiscard]] auto inline make_scope(Args&&... args)
{
  return Scope<T>{ new T{ std::forward<Args>(args)... } };
}

template<class T, Detail::Deleter D, typename... Args>
[[nodiscard]] auto inline make_scope(Args&&... args)
{
  return Scope<T, D>{ new T{ std::forward<Args>(args)... } };
}

template<class T>
using Ref = std::shared_ptr<T>;

template<class T, typename... Args>
[[nodiscard]] auto inline make_ref(Args&&... args)
{
  return Ref<T>{ new T{ std::forward<Args>(args)... } };
}

template<class T>
using Maybe = std::optional<T>;

template<class T, Core::usize PadBytes>
struct Padded
{
  T value;
  std::array<Core::u8, PadBytes> padding{};
  explicit(false) Padded(T v = T{})
    : value(v)
  {
  }

  auto operator=(T new_value) -> void { value = new_value; }

  explicit(false) operator T() const { return value; }
};

using PaddedBool = Padded<bool, 3>;
static_assert(sizeof(PaddedBool) == 4);
static_assert(alignof(PaddedBool) == 1);

using PaddedU32 = Padded<Core::u32, 12>;
static_assert(sizeof(PaddedU32) == 16);
static_assert(alignof(PaddedU32) == 4);

} // namespace Core
