#include "pch/CorePCH.hpp"

#include "ui/UI.hpp"

#include "graphics/InterfaceSystem.hpp"

#include <backends/imgui_impl_vulkan.h>
#include <format>
#include <initializer_list>
#include <span>

namespace Engine::Core::UI {

namespace {

static unsigned int ui_id_counter = 0;
static char ui_id_buffer[10] = "ID";

auto
generate_id() -> const char*
{
  std::ostringstream stream;
  stream << "ID" << std::hex
         << ui_id_counter++; // Convert to hexadecimal and increment
  std::string id_str = stream.str();

  // Ensure the buffer is large enough for the string and a null terminator
  size_t buffer_length = sizeof(ui_id_buffer);
  if (id_str.length() + 1 <= buffer_length) {
    std::copy(id_str.begin(), id_str.end(), ui_id_buffer);
    ui_id_buffer[id_str.length()] = '\0'; // Null-terminate the string
  } else {
    // Handle error or buffer overflow
  }

  return ui_id_buffer;
}

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
convert_to_imvec(const Core::FloatExtent& extent)
{
  return ImVec2{ extent.width, extent.height };
}

auto
to_vector(const ImVec2& vec)
{
  return Core::Vec2{ vec.x, vec.y };
}

template<typename Range>
  requires(std::ranges::range<Range>)
auto
join(Range&& range, std::string_view separator, auto&& formatter) -> std::string
{
  auto begin = std::ranges::begin(range);
  auto end = std::ranges::end(range);
  std::string result;
  if (begin != end) {
    result += formatter(*begin++);
  }
  while (begin != end) {
    result += separator;
    result += formatter(*begin++);
  }
  return result;
}

template<typename... Args>
auto
make_id(Args&&... data)
{
  // Use a fold expression to concatenate the formatted pointer strings
  auto as_string =
    join(std::initializer_list<const void*>{ (&data)... },
         ",",
         [](const auto& const_void_ptr_to_format) {
           return std::format("{}", (const void*)const_void_ptr_to_format);
         });
  return std::format("ID:{}", as_string);
}

}

auto
add_image(VkSampler sampler,
          VkImageView image_view,
          VkImageLayout layout) -> VkDescriptorSet
{
  auto pool = Graphics::InterfaceSystem::get_image_pool();
  auto set = ImGui_ImplVulkan_AddTexture(sampler, image_view, layout, pool);
  return set;
}

auto
push_id() -> void
{
  ImGui::PushID(generate_id());
}

auto
pop_id() -> void
{
  ImGui::PopID();
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

auto
image(const Graphics::Image& image,
      const Core::FloatExtent& extent,
      const Core::Vec4& colour,
      bool flipped,
      Core::u32 array_index) -> void
{
  VkImageView view{ nullptr };
  const auto& [sampler, v, layout] = image.get_descriptor_info();
  if (image.get_layer_count() > 1) {
    view = image.get_layer_image_view(array_index);
  } else {
    view = v;
  }

  auto set = add_image(sampler, view, layout);
  auto made = make_id(set, sampler, view, layout, image.hash());
  ImGui::PushID(made.c_str());
  static constexpr auto uv0 = ImVec2(0, 0);
  static constexpr auto uv1 = ImVec2(1, 1);
  ImGui::Image(set,
               convert_to_imvec(extent),
               flipped ? uv1 : uv0,
               flipped ? uv0 : uv1,
               convert_to_imvec(colour));
  ImGui::PopID();
}

}

} // namespace Engine::UI
