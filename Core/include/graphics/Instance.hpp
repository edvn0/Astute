#pragma once

#include "core/Types.hpp"

#include <vector>
#include <vulkan/vulkan.h>

namespace Engine::Graphics {

class Instance
{
public:
  static auto the() -> Instance&;
  static auto destroy() -> void;
  static auto initialise() -> void;
  auto instance() const -> const VkInstance& { return vk_instance; }

private:
  auto deinitialise() -> void;

  Instance();
  Instance(const Instance&) = delete;
  Instance& operator=(const Instance&) = delete;

  auto check_validation_layer_support(const std::vector<const char*>&) -> bool;
  auto create_instance() -> void;
  auto setup_debug_callback() -> void;

  static inline Core::Scope<Instance> impl;
  static inline bool is_initialised{ false };

  VkInstance vk_instance;
  VkDebugUtilsMessengerEXT debug_messenger;

  const std::vector<const char*> validation_layers = {
    "VK_LAYER_KHRONOS_validation"
  };
};

} // namespace Engine::Core
