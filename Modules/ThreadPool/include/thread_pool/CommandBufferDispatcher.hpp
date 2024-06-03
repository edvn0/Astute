#pragma once

#include "core/Forward.hpp"
#include "core/Types.hpp"
#include "graphics/Forward.hpp"

#include <functional>
#include <queue>
#include <vector>

#include <vulkan/vulkan.h>

namespace ED {

class CommandBufferDispatcher
{
public:
  explicit CommandBufferDispatcher(Engine::Graphics::CommandBuffer&,
                                   VkRenderPass = nullptr,
                                   VkFramebuffer = nullptr);

  template<typename F>
    requires std::invocable<F, Engine::Graphics::CommandBuffer*>
  auto dispatch(F&& func) -> void
  {
    tasks.push(std::forward<F>(func));
  }

  auto execute(bool = true) -> void;

private:
  Engine::Graphics::CommandBuffer* command_buffer;
  std::queue<std::function<void(Engine::Graphics::CommandBuffer*)>> tasks{};

  auto construct_secondary()
    -> Engine::Core::Scope<Engine::Graphics::CommandBuffer>;

  VkCommandBufferInheritanceInfo inheritance_info{};
  VkCommandBufferBeginInfo inherited_begin_info{};
};

} // namespace ED
