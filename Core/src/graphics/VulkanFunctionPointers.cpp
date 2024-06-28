#include "graphics/VulkanFunctionPointers.hpp"

#include "graphics/Instance.hpp"
#include "logging/Logger.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace Engine::Graphics {

PFN_vkCmdBeginDebugUtilsLabelEXT vk_cmd_begin_debug_utils_label_ext{ nullptr };
PFN_vkCmdEndDebugUtilsLabelEXT vk_cmd_end_debug_utils_label_ext{ nullptr };
PFN_vkCmdInsertDebugUtilsLabelEXT vk_cmd_insert_debug_utils_label_ext{
  nullptr
};

auto
VulkanFunctionPointers::initialise(VkInstance instance) -> void
{
  vk_cmd_begin_debug_utils_label_ext =
    reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
      vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
  vk_cmd_end_debug_utils_label_ext =
    reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
      vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));
  vk_cmd_insert_debug_utils_label_ext =
    reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(
      vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT"));
}

auto
VulkanFunctionPointers::insert_gpu_performance_marker(
  VkCommandBuffer command_buffer,
  const std::string_view label,
  const glm::vec4& color) -> void
{
  if (!Instance::uses_validation_layers()) {
    return;
  }

  VkDebugUtilsLabelEXT debug_label{};
  debug_label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
  std::memcpy(&debug_label.color, glm::value_ptr(color), sizeof(float) * 4);
  debug_label.pLabelName = label.data();
  vk_cmd_insert_debug_utils_label_ext(command_buffer, &debug_label);
}

auto
VulkanFunctionPointers::begin_gpu_performance_marker(
  VkCommandBuffer command_buffer,
  const std::string_view label,
  const glm::vec4& marker_colour) -> void
{
  if (!Instance::uses_validation_layers()) {
    return;
  }

  VkDebugUtilsLabelEXT debug_label{};
  debug_label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
  std::memcpy(
    &debug_label.color, glm::value_ptr(marker_colour), sizeof(float) * 4);
  debug_label.pLabelName = label.data();
  vk_cmd_begin_debug_utils_label_ext(command_buffer, &debug_label);
}

auto
VulkanFunctionPointers::end_gpu_performance_marker(
  VkCommandBuffer command_buffer) -> void
{
  if (!Instance::uses_validation_layers()) {
    return;
  }

  vk_cmd_end_debug_utils_label_ext(command_buffer);
}

} // namespace Engine::Graphics
