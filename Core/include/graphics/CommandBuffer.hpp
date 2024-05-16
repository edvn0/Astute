#pragma once

#include "core/Types.hpp"
#include "graphics/Device.hpp"

#include <optional>

#include <vulkan/vulkan.h>

namespace Engine::Graphics {

class CommandBuffer
{
public:
  struct Properties
  {
    const QueueType queue_type;
    const bool owned_by_swapchain{ false };
    const bool primary{ true };
    const std::optional<Core::u32> image_count{ std::nullopt };
  };

  explicit CommandBuffer(const Properties&);
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
  const bool image_count_from_application{ true };

  VkCommandPool command_pool{ nullptr };
  VkQueue queue{ nullptr }; // Owned by device
  VkCommandBuffer active_command_buffer{ nullptr };
  Core::u32 current_frame_index{ 0 };
  std::vector<VkCommandBuffer> command_buffers;
  std::vector<VkFence> fences;

  auto create_command_pool() -> void;
  auto create_command_buffers() -> void;
  auto create_fences() -> void;

  auto destroy() -> void;

  auto is_secondary() const -> bool { return !primary; } 
};

} // namespace Engine::Graphics
