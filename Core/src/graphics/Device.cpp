#include "pch/CorePCH.hpp"

#include "graphics/Device.hpp"
#include "graphics/Instance.hpp"

namespace Engine::Graphics {

auto
Device::is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface)
  -> bool
{
  Core::i32 graphics_family_index = -1;
  Core::i32 present_family_index = -1;
  Core::i32 compute_family_index = -1;
  Core::i32 transfer_family_index = -1;

  Core::u32 family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);
  std::vector<VkQueueFamilyProperties> families(family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(
    device, &family_count, families.data());

  for (int i = 0; i < families.size(); i++) {

    if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      graphics_family_index = i;
    }

    if (families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
      compute_family_index = i;
    }

    if (families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
      transfer_family_index = i;
    }

    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

    if (present_support == VK_TRUE) {
      present_family_index = i;
    }
  }

  bool is_suitable = graphics_family_index >= 0;

  // TODO: Also make sure the queues are sufficiently distinct

  // TODO: Add more suitability checks here (features, extensions, etc.)

  if (is_suitable) {
    queue_support[QueueType::Graphics] = {
      VK_NULL_HANDLE,
      static_cast<Core::u32>(graphics_family_index),
    };
    if (compute_family_index >= 0) {
      queue_support[QueueType::Compute] = {
        VK_NULL_HANDLE,
        static_cast<Core::u32>(compute_family_index),
      };
    }
    if (transfer_family_index >= 0) {
      queue_support[QueueType::Transfer] = {
        VK_NULL_HANDLE,
        static_cast<Core::u32>(transfer_family_index),
      };
    }
    if (present_family_index >= 0) {
      queue_support[QueueType::Present] = {
        VK_NULL_HANDLE,
        static_cast<Core::u32>(present_family_index),
      };
    }
  }

  return is_suitable;
}

class NoDeviceFoundException : public std::runtime_error
{
public:
  NoDeviceFoundException(const std::string_view data)
    : std::runtime_error(data.data())
  {
  }
};

class CouldNotSelectPhysicalException : public std::runtime_error
{
public:
  CouldNotSelectPhysicalException(const std::string_view data)
    : std::runtime_error(data.data())
  {
  }
};

class CouldNotCreateDeviceException : public std::runtime_error
{
public:
  CouldNotCreateDeviceException(const std::string_view data)
    : std::runtime_error(data.data())
  {
  }
};

Device::Device(VkSurfaceKHR surf)
{
  create_device(surf);
}

namespace {

auto
create_queue_info_create_structures(
  const std::unordered_map<Engine::Graphics::QueueType,
                           Engine::Graphics::QueueInformation>& support)
{
  std::vector<VkDeviceQueueCreateInfo> result;
  std::unordered_set<Core::u32> unique_indices;
  for (const auto& [type, info] : support) {
    if (unique_indices.contains(info.family_index)) {
      continue;
    }
    VkDeviceQueueCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    create_info.queueFamilyIndex = info.family_index;
    create_info.queueCount = 1;
    static float prio = 1.0f;
    create_info.pQueuePriorities = &prio;
    result.push_back(create_info);
    unique_indices.insert(info.family_index);
  }

  return result;
}

}
auto
Device::create_device(VkSurfaceKHR surface) -> void
{
  auto& instance = Instance::the().instance();
  Core::u32 device_count = 0;
  vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

  if (device_count == 0) {
    throw NoDeviceFoundException{
      "No devices found",
    };
  }

  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

  for (const auto& device : devices) {
    if (is_device_suitable(device, surface)) {
      vk_physical_device = device;
      break;
    }
  }

  if (!vk_physical_device) {
    throw CouldNotSelectPhysicalException{
      "Failed to find a suitable GPU",
    };
  }

  Core::u32 queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(
    vk_physical_device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(
    vk_physical_device, &queue_family_count, families.data());

  auto queue_infos = create_queue_info_create_structures(queue_support);

  std::vector<const char*> device_extensions;
  device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  VkPhysicalDeviceFeatures device_features{};
  VkDeviceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pQueueCreateInfos = queue_infos.data();
  create_info.queueCreateInfoCount = static_cast<Core::u32>(queue_infos.size());
  create_info.pEnabledFeatures = &device_features;
  create_info.enabledExtensionCount =
    static_cast<Core::u32>(device_extensions.size());
  create_info.ppEnabledExtensionNames = device_extensions.data();

  if (vkCreateDevice(vk_physical_device, &create_info, nullptr, &vk_device) !=
      VK_SUCCESS) {
    throw CouldNotCreateDeviceException{
      "Could not create VkDevice",
    };
  }

  vkGetDeviceQueue(vk_device,
                   queue_support.at(QueueType::Graphics).family_index,
                   0,
                   &queue_support.at(QueueType::Graphics).queue);
  vkGetDeviceQueue(vk_device,
                   queue_support.at(QueueType::Present).family_index,
                   0,
                   &queue_support.at(QueueType::Present).queue);
  vkGetDeviceQueue(vk_device,
                   queue_support.at(QueueType::Compute).family_index,
                   0,
                   &queue_support.at(QueueType::Compute).queue);
  vkGetDeviceQueue(vk_device,
                   queue_support.at(QueueType::Transfer).family_index,
                   0,
                   &queue_support.at(QueueType::Transfer).queue);
}

auto
Device::deinitialise() -> void
{
  vkDestroyDevice(vk_device, nullptr);
}

auto
Device::destroy() -> void
{
  impl->deinitialise();
  impl.reset();
}

auto
Device::the() -> Device&
{
  initialise();
  return *impl;
}

auto
Device::initialise(VkSurfaceKHR surface) -> void
{
  if (!is_initialised) {
    impl = Core::Scope<Device>{ new Device(surface) };
    is_initialised = true;
  }
}

} // namespace Engine::Graphics
