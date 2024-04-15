#pragma once

#include "core/Types.hpp"
#include "graphics/Device.hpp"

#include <vulkan/vulkan.h>

namespace Engine::Graphics {

class CommandBuffer
{
public:
  struct Properties
  {
    const Core::u32 image_count;
    const QueueType queue_type;
    const bool owned_by_swapchain{ false };
    const bool primary{ true };
  };

  explicit CommandBuffer(Properties);
  ~CommandBuffer();

  auto begin(const VkCommandBufferBeginInfo* = nullptr) -> void;
  auto end() -> void;
  auto submit() -> void;

  auto get_command_buffer() const -> VkCommandBuffer
  {
    return active_command_buffer;
  }

private:
  const Core::u32 image_count;
  const Core::u32 queue_family_index;
  const bool owned_by_swapchain;
  const bool primary;

  VkCommandPool command_pool{ nullptr };
  VkQueue queue{ nullptr }; // Owned by device
  VkCommandBuffer active_command_buffer{ nullptr };
  std::vector<VkCommandBuffer> command_buffers;
  std::vector<VkFence> fences;

  auto create_command_pool() -> void;
  auto create_command_buffers() -> void;
  auto create_fences() -> void;

  auto destroy() -> void;
};

} // namespace Engine::Graphics
