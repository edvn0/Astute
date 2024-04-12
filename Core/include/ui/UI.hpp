#pragma once

#include "core/Maths.hpp"

#include <format>
#include <imgui.h>

namespace Engine::UI {

namespace Detail {
template<std::floating_point T, Core::usize N>
  requires(N > 0 && N <= 4)
auto
convert_to_imvec(const Core::Vector<T, N>& vector)
{
  ImVec4 result;

  // Always set x; all your Vector instances have at least one component
  result.x = static_cast<float>(vector.data.at(0));
  result.y = N > 1 ? static_cast<float>(vector.data.at(1)) : 0.0f;
  result.z = N > 2 ? static_cast<float>(vector.data.at(2)) : 0.0f;
  result.w = N > 3 ? static_cast<float>(vector.data.at(3)) : 0.0f;

  return result;
}
}

template<typename... Args>
auto
coloured_text(const Core::Vec4& colour,
              std::format_string<Args...> format,
              Args&&... args)
{
  auto converted = Detail::convert_to_imvec(colour);
  std::string formatted = std::format(format, std::forward<Args>(args)...);
  return ImGui::TextColored(converted, "%s", formatted.c_str());
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
  return ImGui::Begin(formatted.c_str());
}

inline auto
end()
{
  return ImGui::End();
}

inline auto
scope(const std::string_view name, auto&& func)
{
  ImGui::Begin(name.data());
  const auto size = ImGui::GetWindowSize();
  func(size.x, size.y);
  ImGui::End();
}

} // namespace Engine::UI
