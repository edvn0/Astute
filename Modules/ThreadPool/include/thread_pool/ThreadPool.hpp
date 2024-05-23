#pragma once

#include <BS_thread_pool.hpp>

#include <vulkan/vulkan.h>

#include "core/Forward.hpp"
#include "graphics/Forward.hpp"

#include <concepts>
#include <memory>
#include <mutex>
#include <vector>

namespace ED {

template<class T>
concept DeviceProvider = requires(T t) {
  { t.get_device() } -> std::same_as<VkDevice>;
  { t.get_queue_type() } -> std::same_as<Engine::Graphics::QueueType>;
};

class ThreadPool
{
public:
  explicit ThreadPool(
    const DeviceProvider auto device_provider,
    std::uint32_t thread_count = std::thread::hardware_concurrency())
    : device(device_provider.get_device())
  {
    initialise(thread_count,
               device_provider.get_device(),
               device_provider.get_queue_type());
  }
  ~ThreadPool();

  template<typename F>
  auto enqueue_task(F&& f) -> std::future<decltype(f())>
  {
    return thread_pool.submit_task(std::forward<F>(f));
  }

  template<typename Container, typename F>
  auto enqueue_loop_split(Container& container, F&& f)
  {
    using IndexType = decltype(container.size());
    return thread_pool.submit_loop(
      IndexType{ 0 }, container.size(), std::forward<F>(f));
  }

  template<typename F>
  auto enqueue_command_buffer_task(F&& f)
    -> std::future<
      decltype(f(std::declval<Engine::Graphics::CommandBuffer&>()))>
  {
    auto* this_ptr = this;
    auto task = [f = std::forward<F>(f),
                 &buffers = command_buffers,
                 &m = big_lock,
                 this_ptr]() {
      const auto current_thread_index = BS::this_thread::get_index();
      if (!current_thread_index)
        throw std::runtime_error("No thread index");

      auto cmd_buffer = buffers[*current_thread_index].get();
      this_ptr->begin(*cmd_buffer);

      if constexpr (std::is_same_v<decltype(f(*cmd_buffer)), void>) {
        f(*cmd_buffer);
        this_ptr->end(*cmd_buffer);
        {
          std::lock_guard lock(m);
          this_ptr->submit(*cmd_buffer);
        }
      } else {
        auto computed = f(*cmd_buffer);
        this_ptr->end(*cmd_buffer);
        {
          std::lock_guard lock(m);
          this_ptr->submit(*cmd_buffer);
        }
        return computed;
      }
    };

    return thread_pool.submit_task(task);
  }

private:
  BS::thread_pool thread_pool;
  std::vector<std::unique_ptr<Engine::Graphics::CommandBuffer>> command_buffers;
  std::mutex big_lock;
  VkDevice device;
  auto initialise(std::uint32_t, VkDevice, Engine::Graphics::QueueType) -> void;
  auto begin(Engine::Graphics::CommandBuffer&) -> void;
  auto end(Engine::Graphics::CommandBuffer&) -> void;
  auto submit(Engine::Graphics::CommandBuffer&) -> void;
};

} // namespace ED
