#pragma once

#include "core/Maths.hpp"

#include <format>
#include <imgui.h>

namespace Engine::UI {

namespace Impl {
auto
coloured_text(const Core::Vec4&, std::string&&) -> void;

auto
begin(const std::string_view) -> bool;

auto
end() -> void;

auto
get_window_size() -> Core::Vec2;

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
  func(size.x, size.y);
  end();
}

} // namespace Engine::UI
