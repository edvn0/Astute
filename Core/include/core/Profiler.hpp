#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace Engine::Core {

class Profiler
{
public:
  static auto the() -> Profiler&;

  void begin_session(const std::string& name);
  void end_session();
  void write_profile(const std::string& name,
                     std::chrono::steady_clock::time_point start,
                     std::chrono::steady_clock::time_point end);

private:
  struct Entry
  {
    std::string name;
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point end;
  };

  Profiler();
  ~Profiler();
  void write_to_file();

  std::string session_name;
  std::vector<Entry> intermediate_buffer;
  std::mutex profiler_mutex;
  std::mutex cv_mutex;
  std::atomic_bool is_running;
  std::condition_variable cv;
  std::jthread writer_thread;
};

class ProfileScope
{
public:
  explicit ProfileScope(std::string_view name);
  ~ProfileScope();

private:
  std::string name;
  std::chrono::steady_clock::time_point start_point;
};

}

#ifdef ASTUTE_DEBUG

#define CONCATENATE_DETAIL(x, y) x##y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)
#define ASTUTE_PROFILE_FUNCTION()                                              \
  Engine::Core::ProfileScope CONCATENATE(profile_scope_, __LINE__)(__FUNCTION__)

#define ASTUTE_PROFILE_SCOPE(name)                                             \
  Engine::Core::ProfileScope CONCATENATE(profile_scope_, __LINE__)(name)

#else

#define ASTUTE_PROFILE_FUNCTION()
#define ASTUTE_PROFILE_SCOPE(name)

#endif
