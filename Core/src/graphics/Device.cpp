#include "pch/CorePCH.hpp"

#include "core/Logger.hpp"
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
    using enum QueueType;
    queue_support[Graphics] = {
      VK_NULL_HANDLE,
      static_cast<Core::u32>(graphics_family_index),
    };
    if (compute_family_index >= 0) {
      queue_support[Compute] = {
        VK_NULL_HANDLE,
        static_cast<Core::u32>(compute_family_index),
      };
    }
    if (transfer_family_index >= 0) {
      queue_support[Transfer] = {
        VK_NULL_HANDLE,
        static_cast<Core::u32>(transfer_family_index),
      };
    }
    if (present_family_index >= 0) {
      queue_support[Present] = {
        VK_NULL_HANDLE,
        static_cast<Core::u32>(present_family_index),
      };
    }
  }

  // Check for aniostropy support
  VkPhysicalDeviceFeatures supported_features;
  vkGetPhysicalDeviceFeatures(device, &supported_features);
  if (!supported_features.samplerAnisotropy) {
    is_suitable = false;
  }

  if (!supported_features.logicOp) {
    is_suitable = false;
  }

  if (!supported_features.wideLines) {
    is_suitable = false;
  }

  // IS DISCRETE!
  VkPhysicalDeviceProperties device_properties;
  vkGetPhysicalDeviceProperties(device, &device_properties);
  if (device_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    is_suitable = false;
  }

  return is_suitable;
}

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
    throw Core::NoDeviceFoundException{
      "No devices found",
    };
  }

  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

  for (const auto& device : devices) {
    if (is_device_suitable(device, surface)) {
      vk_physical_device = device;

      VkPhysicalDeviceProperties device_properties;
      vkGetPhysicalDeviceProperties(vk_physical_device, &device_properties);
      std::string device_name = device_properties.deviceName;

      info("Chose device: {}", device_name);

      break;
    }
  }

  if (!vk_physical_device) {
    throw Core::CouldNotSelectPhysicalException{
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
  device_features.samplerAnisotropy = VK_TRUE;
  device_features.logicOp = VK_TRUE;
  device_features.wideLines = VK_TRUE;

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
    throw Core::CouldNotCreateDeviceException{
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

  VkCommandPoolCreateInfo command_pool_create_info = {};
  command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  command_pool_create_info.queueFamilyIndex =
    queue_support.at(QueueType::Graphics).family_index;
  vkCreateCommandPool(
    device(), &command_pool_create_info, nullptr, &graphics_command_pool);

  command_pool_create_info.queueFamilyIndex =
    queue_support.at(QueueType::Compute).family_index;
  vkCreateCommandPool(
    device(), &command_pool_create_info, nullptr, &compute_command_pool);
}

auto
Device::execute_immediate(QueueType type,
                          std::function<void(VkCommandBuffer)>&& command)
  -> void
{
  vkResetCommandPool(device(),
                     type == QueueType::Compute ? compute_command_pool
                                                : graphics_command_pool,
                     0);
  VkCommandBuffer command_buffer;

  VkCommandBufferAllocateInfo allocation_info = {};
  allocation_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocation_info.commandPool =
    type == QueueType::Compute ? compute_command_pool : graphics_command_pool;
  allocation_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocation_info.commandBufferCount = 1;

  vkAllocateCommandBuffers(device(), &allocation_info, &command_buffer);

  VkCommandBufferBeginInfo command_buffer_begin_info{};
  command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

  auto func = std::move(command);
  func(command_buffer);
  vkEndCommandBuffer(command_buffer);
  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  VkFenceCreateInfo fence_create_info = {};
  fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_create_info.flags = 0;
  VkFence fence;

  vkCreateFence(device(), &fence_create_info, nullptr, &fence);

  // Submit to queue
  auto queue = get_queue(type);
  vkQueueSubmit(queue, 1, &submit_info, fence);
  static constexpr auto default_fence_timeout = 100000000000;
  vkWaitForFences(device(), 1, &fence, VK_TRUE, default_fence_timeout);

  vkDestroyFence(device(), fence, nullptr);
  vkFreeCommandBuffers(
    device(), allocation_info.commandPool, 1, &command_buffer);
}

auto
Device::create_secondary_command_buffer() -> VkCommandBuffer
{
  VkCommandBufferAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = graphics_command_pool;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(device(), &alloc_info, &command_buffer);

  return command_buffer;
}

auto
Device::reset_command_pools() -> void
{
  vkResetCommandPool(device(), graphics_command_pool, 0);
  vkResetCommandPool(device(), compute_command_pool, 0);
}

auto
Device::deinitialise() -> void
{
  vkDestroyCommandPool(vk_device, graphics_command_pool, nullptr);
  vkDestroyCommandPool(vk_device, compute_command_pool, nullptr);

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
