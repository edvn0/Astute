#pragma once

#include <atomic>
#include <condition_variable>
#include <format>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace ED::Logging {

enum class LogLevel
{
  Trace,
  Debug,
  Info,
  Warn,
  Error,
  None // To disable logging
};

struct BackgroundLogMessage
{
  std::string message;
  LogLevel level{ LogLevel::None };
};

class Logger
{
public:
  static auto get_instance() -> Logger&;
  static auto stop() -> void;
  ~Logger();

  auto set_level(LogLevel level) -> void;
  auto get_level() const -> LogLevel;

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  template<typename... Args>
  void info(std::format_string<Args...> format, Args&&... args) noexcept;

  template<typename... Args>
  void debug(std::format_string<Args...> format, Args&&... args) noexcept;

  template<typename... Args>
  void trace(std::format_string<Args...> format, Args&&... args) noexcept;

  template<typename... Args>
  void warn(std::format_string<Args...> format, Args&&... args) noexcept;

  template<typename... Args>
  void error(std::format_string<Args...> format, Args&&... args) noexcept;

  void log(std::string&& message, LogLevel level);

private:
  Logger();

  void stop_all();
  void process_queue(const std::stop_token&);

  LogLevel current_level{ LogLevel::None };
  static void process_single(const BackgroundLogMessage& message);
  static LogLevel get_log_level_from_environment();

  std::queue<BackgroundLogMessage> log_queue;
  std::mutex queue_mutex;
  std::condition_variable cv;
  std::jthread worker;
  std::atomic_bool exit_flag;
};

}

template<typename... Args>
void
info(std::format_string<Args...> format, Args&&... args) noexcept
{
  ED::Logging::Logger::get_instance().info(format, std::forward<Args>(args)...);
}

template<typename... Args>
void
debug(std::format_string<Args...> format, Args&&... args) noexcept
{
  ED::Logging::Logger::get_instance().debug(format,
                                            std::forward<Args>(args)...);
}

template<typename... Args>
void
trace(std::format_string<Args...> format, Args&&... args) noexcept
{
  ED::Logging::Logger::get_instance().trace(format,
                                            std::forward<Args>(args)...);
}

template<typename... Args>
void
warn(std::format_string<Args...> format, Args&&... args) noexcept
{
  ED::Logging::Logger::get_instance().warn(format, std::forward<Args>(args)...);
}

template<typename... Args>
void
error(std::format_string<Args...> format, Args&&... args) noexcept
{
  ED::Logging::Logger::get_instance().error(format,
                                            std::forward<Args>(args)...);
}

#include "logging/Logger.inl"
