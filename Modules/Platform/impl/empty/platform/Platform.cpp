#include "platform/Platform.hpp"

#include <array>

namespace ED::Platform {

auto
get_system_name() -> std::string
{
  static constexpr std::string_view fallback = "default";
  return std::string(fallback);
}

auto
get_environment_variable(const std::string_view) -> std::string
{
  return std::string("Missing");
}

} // namespace Core::Platform
