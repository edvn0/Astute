#include "pch/CorePCH.hpp"

#include "core/Application.hpp"
#include "graphics/CommandBuffer.hpp"
#include "graphics/Swapchain.hpp"

#include "logging/Logger.hpp"

namespace Engine::Graphics {

CommandBuffer::CommandBuffer(const Properties& props)

  : image_count_from_application(props.image_count.has_value() ? false : true)
  , image_count(props.image_count.has_value()
                  ? *props.image_count
                  : Core::Application::the().get_image_count())
  , queue_family_index(Device::the().get_family(props.queue_type))
  , owned_by_swapchain(props.owned_by_swapchain)
  , primary(props.primary)
  , queue(Device::the().get_queue(props.queue_type))
{
  if (owned_by_swapchain) {
    return;
  }

  create_command_pool();
  create_command_buffers();
  create_fences();
}

CommandBuffer::~CommandBuffer()
{
  if (owned_by_swapchain)
    return;

  destroy();
}

auto
CommandBuffer::destroy() -> void
{
  for (auto& fence : fences) {
    vkDestroyFence(Device::the().device(), fence, nullptr);
  }
  vkDestroyCommandPool(Device::the().device(), command_pool, nullptr);
}

auto
CommandBuffer::create_command_pool() -> void
{
  VkCommandPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
                    VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  pool_info.queueFamilyIndex = queue_family_index;

  vkCreateCommandPool(
    Device::the().device(), &pool_info, nullptr, &command_pool);
}

auto
CommandBuffer::create_command_buffers() -> void
{
  command_buffers.resize(image_count);
  VkCommandBufferAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = command_pool;
  alloc_info.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY
                             : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  alloc_info.commandBufferCount = image_count;

  vkAllocateCommandBuffers(
    Device::the().device(), &alloc_info, command_buffers.data());
}

auto
CommandBuffer::create_fences() -> void
{
  fences.resize(image_count);
  VkFenceCreateInfo fence_info = {};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (auto& fence : fences) {
    vkCreateFence(Device::the().device(), &fence_info, nullptr, &fence);
  }
}

auto
CommandBuffer::begin(const VkCommandBufferBeginInfo* begin_info) -> void
{
  VkCommandBufferBeginInfo default_begin_info = {};
  default_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  default_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  if (begin_info != nullptr) {
    // Copy to default_begin_info
    default_begin_info.flags = begin_info->flags;
    default_begin_info.pInheritanceInfo = begin_info->pInheritanceInfo;
    default_begin_info.pNext = begin_info->pNext;
  }

  current_frame_index = image_count_from_application
                          ? Core::Application::the().current_frame_index()
                          : 0;
  if (owned_by_swapchain) {
    active_command_buffer =
      Core::Application::the().get_swapchain().get_command_buffer(
        current_frame_index);
  } else {
    active_command_buffer = command_buffers[current_frame_index];
  }
  vkBeginCommandBuffer(active_command_buffer, &default_begin_info);
}

auto
CommandBuffer::end() -> void
{
  vkEndCommandBuffer(active_command_buffer);
}

auto
CommandBuffer::submit() -> void
{
  if (owned_by_swapchain) {
    return;
  }

  if (is_secondary()) {
    return;
  }

  const auto& device = Device::the();
  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  VkCommandBuffer buffer = active_command_buffer;
  submit_info.pCommandBuffers = &buffer;
  vkResetFences(device.device(), 1, &fences[current_frame_index]);

  vkQueueSubmit(queue, 1, &submit_info, fences[current_frame_index]);

  vkWaitForFences(
    device.device(), 1, &fences[current_frame_index], VK_TRUE, UINT64_MAX);

  vkResetFences(device.device(), 1, &fences[current_frame_index]);

  active_command_buffer = nullptr;
}

} // namespace Engine::Graphics
