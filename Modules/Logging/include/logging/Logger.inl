#pragma once

#include "logging/Logger.hpp"

namespace ED::Logging {

#ifndef RELEASE
template<typename... Args>
void
Logger::trace(std::format_string<Args...> format, Args&&... args) noexcept
{
  if (current_level > LogLevel::Trace)
    return;
  log(std::format(format, std::forward<Args>(args)...), LogLevel::Trace);
}

template<typename... Args>
void
Logger::debug(std::format_string<Args...> format, Args&&... args) noexcept
{
  if (current_level > LogLevel::Debug)
    return;
  log(std::format(format, std::forward<Args>(args)...), LogLevel::Debug);
}

#else
template<typename... Args>
void
Logger::trace(std::format_string<Args...> format, Args&&... args) noexcept
{
}

template<typename... Args>
void
Logger::debug(std::format_string<Args...> format, Args&&... args) noexcept
{
}

#endif

template<typename... Args>
void
Logger::info(std::format_string<Args...> format, Args&&... args) noexcept
{
  if (current_level > LogLevel::Info)
    return;
  log(std::format(format, std::forward<Args>(args)...), LogLevel::Info);
}

template<typename... Args>
void
Logger::warn(std::format_string<Args...> format, Args&&... args) noexcept
{
  log(std::format(format, std::forward<Args>(args)...), LogLevel::Warn);
}

template<typename... Args>
void
Logger::error(std::format_string<Args...> format, Args&&... args) noexcept
{
  log(std::format(format, std::forward<Args>(args)...), LogLevel::Error);
}

} // namespace Core
