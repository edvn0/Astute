#pragma once

#include "core/Types.hpp"

#include <vulkan/vulkan.h>

namespace Engine::Graphics {

enum class QueueType : Core::u8
{
  Graphics,
  Compute,
  Present,
  Transfer
};

struct QueueInformation
{
  VkQueue queue;
  Core::u32 family_index;
};

class Device
{
public:
  static auto the() -> Device&;
  static auto destroy() -> void;
  static auto initialise(VkSurfaceKHR = nullptr) -> void;
  auto device() const -> const VkDevice& { return vk_device; }
  auto device() -> VkDevice { return vk_device; }
  auto physical() -> VkPhysicalDevice { return vk_physical_device; }
  auto physical() const -> const VkPhysicalDevice&
  {
    return vk_physical_device;
  }

  auto get_queue(QueueType t) { return queue_support.at(t).queue; }
  auto get_queue(QueueType t) const { return queue_support.at(t).queue; }
  auto get_family(QueueType t) { return queue_support.at(t).family_index; }
  auto get_family(QueueType t) const
  {
    return queue_support.at(t).family_index;
  }

  auto execute_immediate(QueueType, std::function<void(VkCommandBuffer)>&&)
    -> void;
  auto execute_immediate(std::function<void(VkCommandBuffer)>&& command) -> void
  {
    execute_immediate(QueueType::Graphics, std::move(command));
  }

  auto create_secondary_command_buffer() -> VkCommandBuffer;

private:
  auto deinitialise() -> void;

  explicit Device(VkSurfaceKHR);
  Device(const Device&) = delete;
  Device& operator=(const Device&) = delete;

  auto create_device(VkSurfaceKHR) -> void;
  auto is_device_suitable(VkPhysicalDevice, VkSurfaceKHR) -> bool;

  static inline Core::Scope<Device> impl;
  static inline bool is_initialised{ false };

  VkDevice vk_device;
  VkPhysicalDevice vk_physical_device;
  VkCommandPool graphics_command_pool;
  VkCommandPool compute_command_pool;

  std::unordered_map<QueueType, QueueInformation> queue_support;
};

}
