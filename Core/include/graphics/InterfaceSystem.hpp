#pragma once

#include <functional>
#include <mutex>
#include <queue>

#include "core/FrameBasedCollection.hpp"
#include "graphics/CommandBuffer.hpp"
#include "graphics/Forward.hpp"

extern "C"
{
  struct ImFont;
}

namespace Engine::Graphics {

class InterfaceSystem
{
public:
  explicit InterfaceSystem(const Window& win);
  ~InterfaceSystem();

  auto begin_frame() -> void;
  auto end_frame() -> void;

  static auto get_image_pool() { return image_pool->get(); }

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

  static inline Core::Scope<Core::FrameBasedCollection<VkDescriptorPool>>
    image_pool{};
};

} // namespace Core
