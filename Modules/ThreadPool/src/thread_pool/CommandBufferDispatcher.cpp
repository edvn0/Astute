#include "pch/ThreadPoolPCH.hpp"

#include "thread_pool/CommandBufferDispatcher.hpp"

#include "graphics/CommandBuffer.hpp"

namespace ED {

CommandBufferDispatcher::CommandBufferDispatcher()
{
  command_buffer = Engine::Core::make_scope<Engine::Graphics::CommandBuffer>(
    Engine::Graphics::CommandBuffer::Properties{
      .queue_type = Engine::Graphics::QueueType::Graphics,
      .primary = true,
      .image_count = 1,
    });

  inheritance_info = {};
  inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

  inherited_begin_info = {};
  inherited_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  inherited_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  inherited_begin_info.pInheritanceInfo = &inheritance_info;
}

auto
CommandBufferDispatcher::construct_secondary()
  -> Engine::Core::Scope<Engine::Graphics::CommandBuffer>
{
  return Engine::Core::make_scope<Engine::Graphics::CommandBuffer>(
    Engine::Graphics::CommandBuffer::Properties{
      .queue_type = Engine::Graphics::QueueType::Graphics,
      .primary = false,
      .image_count = 1,
    });
}

auto
CommandBufferDispatcher::execute() -> void
{
  command_buffer->begin();
  std::vector<Engine::Core::Scope<Engine::Graphics::CommandBuffer>>
    secondary_command_buffers;

  while (!tasks.empty()) {
    auto task = tasks.front();
    tasks.pop();

    auto& secondary =
      secondary_command_buffers.emplace_back(construct_secondary());
    secondary->begin(&inherited_begin_info);
    task(secondary.get());
    secondary->end();
  }

  std::vector<VkCommandBuffer> secondary_buffers;
  secondary_buffers.reserve(secondary_command_buffers.size());
  for (const auto& cb : secondary_command_buffers) {
    secondary_buffers.push_back(cb->get_command_buffer());
  }

  vkCmdExecuteCommands(command_buffer->get_command_buffer(),
                       static_cast<Engine::Core::u32>(secondary_buffers.size()),
                       secondary_buffers.data());

  vkEndCommandBuffer(command_buffer->get_command_buffer());

  command_buffer->submit();
}

}