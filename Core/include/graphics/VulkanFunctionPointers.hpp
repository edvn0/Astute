#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <string_view>

namespace Engine::Graphics {

class VulkanFunctionPointers
{
public:
  static auto initialise(VkInstance) -> void;

  static auto begin_gpu_performance_marker(VkCommandBuffer,
                                           std::string_view,
                                           const glm::vec4&) -> void;
  static auto insert_gpu_performance_marker(VkCommandBuffer,
                                            std::string_view,
                                            const glm::vec4&) -> void;
  static auto end_gpu_performance_marker(VkCommandBuffer) -> void;
};

}
