#pragma once

#include <functional>
#include <imgui.h>
#include <mutex>
#include <queue>

#include "graphics/CommandBuffer.hpp"
#include "graphics/Forward.hpp"

namespace Engine::Graphics {

class InterfaceSystem
{
public:
  InterfaceSystem(const Window& win);
  ~InterfaceSystem();

  auto begin_frame() -> void;
  auto end_frame() -> void;

  static auto get_image_pool() { return image_pool; }

private:
  const Window* window{ nullptr };

  VkDescriptorPool pool{};

  struct InterfaceCommandBuffer
  {
    VkCommandBuffer buffer;
  };
  std::vector<InterfaceCommandBuffer> command_buffers;
  ImFont* font{ nullptr };

  std::string system_name;

  static inline VkDescriptorPool image_pool{};
};

} // namespace Core
