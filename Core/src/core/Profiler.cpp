#include "pch/CorePCH.hpp"

#include "core/Types.hpp"

#include "core/Profiler.hpp"

#include <fstream>

namespace Engine::Core {

auto
Profiler::the() -> Profiler&
{
  static Profiler instance;
  return instance;
}

void
Profiler::begin_session(const std::string& name)
{
  (void)name;
#ifdef ASTUTE_DEBUG
  std::lock_guard<std::mutex> lock(profiler_mutex);
  session_name = name;
  intermediate_buffer.clear();
#endif
}

void
Profiler::end_session()
{
#ifdef ASTUTE_DEBUG
  std::lock_guard<std::mutex> lock(profiler_mutex);
  write_to_file();
#endif
}

void
Profiler::write_profile(const std::string& name,
                        std::chrono::steady_clock::time_point start,
                        std::chrono::steady_clock::time_point end)
{
  std::lock_guard<std::mutex> lock(profiler_mutex);
  intermediate_buffer.emplace_back(Entry{ name, start, end });
}

Profiler::Profiler()
  : session_name("")
  , intermediate_buffer()
  , is_running(true)
{
  writer_thread = std::jthread([this]() {
    std::unique_lock<std::mutex> lock(cv_mutex);
    while (is_running) {
      if (cv.wait_for(lock, std::chrono::seconds(5)) ==
          std::cv_status::timeout) {
        write_to_file();
      }
    }
  });
}

Profiler::~Profiler()
{
  {
    std::lock_guard<std::mutex> lock(cv_mutex);
    is_running = false;
  }
  cv.notify_all();
  if (writer_thread.joinable()) {
    writer_thread.join();
  }
  write_to_file();
}

void
Profiler::write_to_file()
{
  std::ofstream file(session_name + ".json");
  if (!file.is_open()) {
    return;
  }

  file << "{\"traceEvents\":[";
  for (size_t i = 0; i < intermediate_buffer.size(); ++i) {
    const auto& entry = intermediate_buffer[i];
    file << (i > 0 ? "," : "") << "{\"name\":\"" << entry.name << "\","
         << "\"ph\":\"X\","
         << "\"ts\":"
         << std::chrono::duration_cast<std::chrono::microseconds>(
              entry.start.time_since_epoch())
              .count()
         << ","
         << "\"dur\":"
         << std::chrono::duration_cast<std::chrono::microseconds>(entry.end -
                                                                  entry.start)
              .count()
         << ","
         << "\"pid\":0,\"tid\":0}";
  }
  file << "]}";
  file.close();
}

ProfileScope::ProfileScope(const std::string_view name)
  : name(name)
  , start_point(std::chrono::steady_clock::now())
{
}

ProfileScope::~ProfileScope()
{
  auto end_point = std::chrono::steady_clock::now();
  Profiler::the().write_profile(name, start_point, end_point);
}

}
