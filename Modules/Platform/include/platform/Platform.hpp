#pragma once

#include <string>
#include <string_view>

namespace ED::Platform {

auto
get_system_name() -> std::string;

auto get_environment_variable(std::string_view) -> std::string;

}
