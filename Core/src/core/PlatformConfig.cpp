#include "core/PlatformConfig.hpp"

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

  std::string multiByteString(length, 0);
  WideCharToMultiByte(CP_UTF8,
                      0,
                      wideCharArray,
                      -1,
                      &multiByteString[0],
                      length,
                      nullptr,
                      nullptr);

  // Remove the null terminator
  multiByteString.pop_back();
  return multiByteString;
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

} // namespace Core::Platform
