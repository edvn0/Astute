#include "pch/CorePCH.hpp"

#include "graphics/Allocator.hpp"

#include "core/Logger.hpp"

#include <cassert>

#include "core/Verify.hpp"
#include "graphics/Device.hpp"
#include "graphics/Instance.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Engine::Graphics {

Allocator::Allocator(const std::string& resource)
  : resource_name(resource)
{
}

auto
Allocator::allocate_buffer(VkBuffer& buffer,
                           VkBufferCreateInfo& buffer_info,
                           const AllocationProperties& props) -> VmaAllocation
{
  // ensure(allocator != nullptr, "Allocator was null.");
  VmaAllocationCreateInfo alloc_info = {};
  alloc_info.usage = static_cast<VmaMemoryUsage>(props.usage);
  alloc_info.flags = static_cast<VmaAllocationCreateFlags>(props.creation);

  VmaAllocation allocation{};
  vmaCreateBuffer(
    allocator, &buffer_info, &alloc_info, &buffer, &allocation, nullptr);
  vmaSetAllocationName(allocator, allocation, resource_name.data());

  return allocation;
}

auto
Allocator::allocate_buffer(VkBuffer& buffer,
                           VmaAllocationInfo& allocation_info,
                           VkBufferCreateInfo& buffer_info,
                           const AllocationProperties& props) -> VmaAllocation
{
  // ensure(allocator != nullptr, "Allocator was null.");
  VmaAllocationCreateInfo alloc_info{};
  alloc_info.usage = static_cast<VmaMemoryUsage>(props.usage);
  alloc_info.flags = static_cast<VmaAllocationCreateFlags>(props.creation) |
                     VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

  VmaAllocation allocation{};
  vmaCreateBuffer(allocator,
                  &buffer_info,
                  &alloc_info,
                  &buffer,
                  &allocation,
                  &allocation_info);
  vmaSetAllocationName(allocator, allocation, resource_name.data());

  return allocation;
}

auto
Allocator::allocate_image(VkImage& image,
                          VkImageCreateInfo& image_create_info,
                          const AllocationProperties& props) -> VmaAllocation
{
  VmaAllocationCreateInfo allocation_create_info = {};
  allocation_create_info.usage = static_cast<VmaMemoryUsage>(props.usage);
  if (props.flags != RequiredFlags::FLAG_BITS_MAX_ENUM) {
    allocation_create_info.flags =
      static_cast<VmaAllocationCreateFlags>(props.flags);
  }

  VmaAllocation allocation{};
  VK_CHECK(vmaCreateImage(allocator,
                          &image_create_info,
                          &allocation_create_info,
                          &image,
                          &allocation,
                          nullptr));
  vmaSetAllocationName(allocator, allocation, resource_name.data());

  return allocation;
}

auto
Allocator::allocate_image(VkImage& image,
                          VmaAllocationInfo& allocation_info,
                          VkImageCreateInfo& image_create_info,
                          const AllocationProperties& props) -> VmaAllocation
{
  VmaAllocationCreateInfo allocation_create_info = {};
  allocation_create_info.usage = static_cast<VmaMemoryUsage>(props.usage);
  if (props.flags != RequiredFlags::FLAG_BITS_MAX_ENUM) {
    allocation_create_info.flags =
      static_cast<VmaAllocationCreateFlags>(props.flags);
  }

  VmaAllocation allocation{};
  VK_CHECK(vmaCreateImage(allocator,
                          &image_create_info,
                          &allocation_create_info,
                          &image,
                          &allocation,
                          &allocation_info));
  vmaSetAllocationName(allocator, allocation, resource_name.data());

  return allocation;
}

void
Allocator::deallocate_buffer(VmaAllocation allocation, VkBuffer& buffer)
{
  vmaDestroyBuffer(allocator, buffer, allocation);
}

void
Allocator::deallocate_image(VmaAllocation allocation, VkImage& image)
{
  // ensure(allocator != nullptr, "Allocator was null.");
  vmaDestroyImage(allocator, image, allocation);
}

void
Allocator::unmap_memory(VmaAllocation allocation)
{
  vmaUnmapMemory(allocator, allocation);
}

auto
Allocator::construct_allocator(const Device& device, const Instance& instance)
  -> VmaAllocator
{
  VmaAllocatorCreateInfo allocator_create_info{};
  allocator_create_info.physicalDevice = device.physical();
  allocator_create_info.device = device.device();
  allocator_create_info.instance = instance.instance();

  VmaAllocator alloc{};
  VK_CHECK(vmaCreateAllocator(&allocator_create_info, &alloc));
  return alloc;
}

} // namespace Core
