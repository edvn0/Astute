#pragma once

#include "core/Maths.hpp"
#include "core/Types.hpp"
#include "graphics/Image.hpp"

#include <format>
#include <imgui.h>

namespace Engine::Core::UI {

template<Number T>
struct InterfaceImageProperties
{
  Core::BasicExtent<T> extent{ T{ 64 }, T{ 64 } };
  Core::Vec4 colour{ 1.0, 1.0, 1.0, 1.0 };
  bool flipped{ false };
  std::optional<Core::u32> image_array_index{};
};

namespace Impl {
auto
coloured_text(const Core::Vec4&, std::string&&) -> void;

auto
begin(const std::string_view) -> bool;

auto
end() -> void;

auto
get_window_size() -> Core::Vec2;

auto
image(const Graphics::Image& image,
      const Core::FloatExtent& extent,
      const Core::Vec4& colour,
      bool flipped,
      Core::u32 array_index) -> void;

}

template<typename... Args>
auto
coloured_text(const Core::Vec4& colour,
              std::format_string<Args...> format,
              Args&&... args)
{
  std::string formatted = std::format(format, std::forward<Args>(args)...);
  return Impl::coloured_text(colour, std::move(formatted));
}

template<typename... Args>
auto
text(std::format_string<Args...> format, Args&&... args)
{
  return coloured_text(
    { 1.0F, 1.0F, 1.0F, 1.0F }, format, std::forward<Args>(args)...);
}

template<typename... Args>
auto
begin(std::format_string<Args...> format, Args&&... args)
{
  std::string formatted = std::format(format, std::forward<Args>(args)...);
  return Impl::begin(formatted.c_str());
}

inline auto
end() -> void
{
  return Impl::end();
}

inline auto
get_window_size() -> Core::Vec2
{
  return Impl::get_window_size();
}

inline auto
scope(const std::string_view name, auto&& func)
{
  Impl::begin(name);
  const auto size = ImGui::GetWindowSize();
  if constexpr (std::is_invocable_v<decltype(func), Core::Vec2>) {
    func({ size.x, size.y });
  } else if constexpr (std::is_invocable_v<decltype(func)>) {
    func();
  } else if constexpr (std::
                         is_invocable_v<decltype(func), Core::f32, Core::f32>) {
    func(size.x, size.y);
  } else {
    func(size);
  }
  end();
}

template<class T>
auto
image(const Graphics::Image& image,
      InterfaceImageProperties<T> properties = {}) -> void
{
  const auto array_index_or_zero = properties.image_array_index.value_or(0);
  if constexpr (std::is_same_v<T, Core::f32>) {
    return Impl::image(image,
                       properties.extent,
                       properties.colour,
                       properties.flipped,
                       array_index_or_zero);
  } else {

    auto ext = properties.extent.template as<Core::f32>();
    return Impl::image(
      image, ext, properties.colour, properties.flipped, array_index_or_zero);
  }
}

} // namespace Engine::UI
