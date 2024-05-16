#include "pch/ThreadPoolPCH.hpp"

#include "thread_pool/ThreadPool.hpp"

#include "graphics/CommandBuffer.hpp"
#include "graphics/Types.hpp"

#include <cassert>

namespace ED {

auto
ThreadPool::initialise(std::uint32_t thread_count,
                       VkDevice device,
                       Engine::Graphics::QueueType type) -> void
{
  mutexes.resize(thread_count);
  for (auto& m : mutexes) {
    m = Engine::Core::make_scope<std::mutex>();
  }

  command_buffers.resize(thread_count);
  // Create a command buffer for each thread
  for (auto& cb : command_buffers) {
    cb = std::make_unique<Engine::Graphics::CommandBuffer>(
      Engine::Graphics::CommandBuffer::Properties{
        .queue_type = type,
        .owned_by_swapchain = false,
        .primary = true,
        .image_count = 1,
      });
  }
}

auto
ThreadPool::begin(Engine::Graphics::CommandBuffer& cb) -> void
{
  cb.begin();
}

auto
ThreadPool::end(Engine::Graphics::CommandBuffer& cb) -> void
{
  cb.end();
}

auto
ThreadPool::submit(Engine::Graphics::CommandBuffer& cb) -> void
{
  cb.submit();
}

}