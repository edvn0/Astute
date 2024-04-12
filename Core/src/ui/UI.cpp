#include "pch/CorePCH.hpp"

#include "ui/UI.hpp"

namespace Engine::UI {

namespace {
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

auto
to_vector(const ImVec4& vec)
{
  return Core::Vec4{ vec.x, vec.y, vec.z, vec.w };
}
auto
to_vector(const ImVec2& vec)
{
  return Core::Vec2{ vec.x, vec.y };
}
}

namespace Impl {

auto
coloured_text(const Core::Vec4& colour, std::string&& formatted) -> void
{
  return ImGui::TextColored(convert_to_imvec(colour), "%s", formatted.c_str());
}

auto
begin(const std::string_view data) -> bool
{
  return ImGui::Begin(data.data());
}

auto
end() -> void
{
  return ImGui::End();
}

auto
get_window_size() -> Core::Vec2
{
  return to_vector(ImGui::GetWindowSize());
}

}

} // namespace Engine::UI
