#include "pch/CorePCH.hpp"

#include "graphics/Instance.hpp"
#include "logging/Logger.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Engine::Graphics {

#ifdef ENABLE_VALIDATION_LAYERS
constexpr auto enable_validation_layers = true;
#else
constexpr auto enable_validation_layers = false;
#endif

static auto
create_debug_utils_messenger_ext(
  VkInstance instance,
  const VkDebugUtilsMessengerCreateInfoEXT* create_info,
  const VkAllocationCallbacks* allocator,
  VkDebugUtilsMessengerEXT* debug_messenger) -> VkResult
{
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
    instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, create_info, allocator, debug_messenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

static auto
destroy_debug_utils_messenger_ext(VkInstance instance,
                                  VkDebugUtilsMessengerEXT debug_messenger,
                                  const VkAllocationCallbacks* allocator)
  -> void
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
    instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debug_messenger, allocator);
  }
}

auto
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
               VkDebugUtilsMessageTypeFlagsEXT,
               const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
               void*) -> VkBool32
{
  using namespace ED::Logging;
  LogLevel log_level = LogLevel::None;
  if ((message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0) {
    log_level = LogLevel::Error;
  } else if ((message_severity &
              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0) {
    log_level = LogLevel::Warn;
  } else if ((message_severity &
              VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) != 0) {
    log_level = LogLevel::Info;
  } else if ((message_severity &
              VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) != 0) {
    log_level = LogLevel::Debug;
  }

  std::string message = "Validation layer: ";
  message += callback_data->pMessage;

  Logger::get_instance().log(std::move(message), log_level);

  return VK_FALSE;
}

auto
Instance::create_instance() -> void
{
  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Astute Application";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "AstuteEngine";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_3;

  VkInstanceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;

  // Get required instance extensions from GLFW
  uint32_t glfw_extensions_count = 0;
  const char** glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);

  std::vector<const char*> extensions(glfw_extensions,
                                      glfw_extensions + glfw_extensions_count);

  if (enable_validation_layers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    info("Enabled validation layers!");
  } else {
    info("Validation layers are disabled!");
  }

  create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  create_info.ppEnabledExtensionNames = extensions.data();

  if (enable_validation_layers) {
    create_info.enabledLayerCount =
      static_cast<Core::u32>(validation_layers.size());
    create_info.ppEnabledLayerNames = validation_layers.data();
  } else {
    create_info.enabledLayerCount = 0;
    create_info.pNext = nullptr;
  }

  if (vkCreateInstance(&create_info, nullptr, &vk_instance) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create Vulkan instance!");
  }
}

auto
Instance::setup_debug_callback() -> void
{
  VkDebugUtilsMessengerCreateInfoEXT create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity =
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.pfnUserCallback = debug_callback;
  create_info.pUserData = nullptr; // Optional

  if (create_debug_utils_messenger_ext(
        vk_instance, &create_info, nullptr, &debug_messenger) != VK_SUCCESS) {
    throw std::runtime_error("Failed to set up debug callback!");
  }
}

auto
Instance::check_validation_layer_support(
  const std::vector<const char*>& input_layers) -> bool
{
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
  std::vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  for (const char* layer_name : input_layers) {
    std::string_view view = { layer_name };
    bool layer_found = false;

    for (const auto& layer_properties : available_layers) {
      std::string_view prop_layer_name = layer_properties.layerName;
      if (prop_layer_name == view) {
        layer_found = true;
        break;
      }
    }

    if (!layer_found) {
      return false;
    }
  }

  return true;
}

Instance::Instance()
{
  if constexpr (enable_validation_layers)
    if (!check_validation_layer_support(validation_layers)) {
      throw std::runtime_error(
        "Validation layers requested, but not available!");
    }

  create_instance();

  if (enable_validation_layers) {
    setup_debug_callback();
  }
}

auto
Instance::deinitialise() -> void
{
  destroy_debug_utils_messenger_ext(vk_instance, debug_messenger, nullptr);
  vkDestroyInstance(vk_instance, nullptr);
}

auto
Instance::destroy() -> void
{
  if (!impl) {
    return;
  }

  impl->deinitialise();
  impl.reset();
}

auto
Instance::the() -> Instance&
{
  if (!is_initialised) {
    initialise();
  }

  return *impl;
}

auto
Instance::initialise() -> void
{
  if (!is_initialised) {
    impl = Core::Scope<Instance>{ new Instance() };
    is_initialised = true;
  }
}

}
