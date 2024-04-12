#include "core/Platform.hpp"

#include <Windows.h>
#include <array>

namespace Engine::Core::Platform {

std::string
wchar_to_string(const WCHAR* wideCharArray)
{
  if (!wideCharArray)
    return "";

  int length = WideCharToMultiByte(
    CP_UTF8, 0, wideCharArray, -1, nullptr, 0, nullptr, nullptr);
  if (length == 0)
    return "";

  std::string multibyte_string(length, 0);
  WideCharToMultiByte(CP_UTF8,
                      0,
                      wideCharArray,
                      -1,
                      &multibyte_string[0],
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
  std::array<WCHAR, 256> wideBuffer{};
  DWORD size = sizeof(wideBuffer) / sizeof(wideBuffer[0]);
  if (!GetComputerNameW(wideBuffer.data(), &size)) {
    return "default"; // Fallback name
  }
  return wchar_to_string(wideBuffer.data());
}

auto
get_environment_variable(const std::string_view var_name) -> std::string
{
  std::string value;

#if defined(_WIN32)
  char* buffer = nullptr;
  size_t size = 0;
  errno_t err = _dupenv_s(&buffer, &size, var_name.data());
  if (!err && buffer != nullptr) {
    value = buffer;
    free(buffer);
  }
#else
  const char* env = std::getenv(var_name.data());
  if (env != nullptr) {
    value = env;
  }
#endif

  return value;
}

} // namespace Core::Platform
