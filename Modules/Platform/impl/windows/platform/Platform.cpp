#include "platform/Platform.hpp"

#include <Windows.h>
#include <array>

namespace ED::Platform {

auto
wchar_to_string(const WCHAR* write_char_array) -> std::string
{
  if (write_char_array == nullptr) {
    return "";
  }

  int length = WideCharToMultiByte(
    CP_UTF8, 0, write_char_array, -1, nullptr, 0, nullptr, nullptr);
  if (length == 0) {
    return "";
  }

  std::string multibyte_string(length, 0);
  WideCharToMultiByte(CP_UTF8,
                      0,
                      write_char_array,
                      -1,
                      multibyte_string.data(),
                      length,
                      nullptr,
                      nullptr);

  // Remove the null terminator
  multibyte_string.pop_back();
  return multibyte_string;
}

auto
get_system_name() -> std::string
{
  std::array<WCHAR, 256> wide_buffer{};
  DWORD size = sizeof(wide_buffer) / sizeof(wide_buffer[0]);
  if (!GetComputerNameW(wide_buffer.data(), &size)) {
    return "default"; // Fallback name
  }
  return wchar_to_string(wide_buffer.data());
}

auto
get_environment_variable(const std::string_view var_name) -> std::string
{
  std::string value;
  char* buffer = nullptr;
  size_t size = 0;
  errno_t err = _dupenv_s(&buffer, &size, var_name.data());
  if (!err && buffer != nullptr) {
    value = buffer;
    free(buffer);
  }
  return value;
}

} // namespace Core::Platform
