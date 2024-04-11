// N.B This class should technically only be included in cpp files.
#pragma once

#include "core/Logger.hpp"
#include "core/Types.hpp"
#include "graphics/Device.hpp"
#include "graphics/Instance.hpp"

#include <vk_mem_alloc.h>

namespace Engine::Graphics {

enum class Usage : Core::u32
{
  UNKNOWN = 0,
  GPU_ONLY = 1,
  CPU_ONLY = 2,
  CPU_TO_GPU = 3,
  GPU_TO_CPU = 4,
  CPU_COPY = 5,
  GPU_LAZILY_ALLOCATED = 6,
  AUTO = 7,
  AUTO_PREFER_DEVICE = 8,
  AUTO_PREFER_HOST = 9,
};
enum class Creation : Core::u32
{
  DEDICATED_MEMORY_BIT = 0x00000001,
  NEVER_ALLOCATE_BIT = 0x00000002,
  MAPPED_BIT = 0x00000004,
  USER_DATA_COPY_STRING_BIT = 0x00000020,
  UPPER_ADDRESS_BIT = 0x00000040,
  DONT_BIND_BIT = 0x00000080,
  WITHIN_BUDGET_BIT = 0x00000100,
  CAN_ALIAS_BIT = 0x00000200,
  HOST_ACCESS_SEQUENTIAL_WRITE_BIT = 0x00000400,
  HOST_ACCESS_RANDOM_BIT = 0x00000800,
  HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT = 0x00001000,
  STRATEGY_MIN_MEMORY_BIT = 0x00010000,
  STRATEGY_MIN_TIME_BIT = 0x00020000,
  STRATEGY_MIN_OFFSET_BIT = 0x00040000,
  STRATEGY_BEST_FIT_BIT = STRATEGY_MIN_MEMORY_BIT,
  STRATEGY_FIRST_FIT_BIT = STRATEGY_MIN_TIME_BIT,
  STRATEGY_MASK =
    STRATEGY_MIN_MEMORY_BIT | STRATEGY_MIN_TIME_BIT | STRATEGY_MIN_OFFSET_BIT,
};

constexpr auto
operator|(Creation left, Creation right) -> Creation
{
  return static_cast<Creation>(static_cast<Core::u32>(left) |
                               static_cast<Core::u32>(right));
}

constexpr auto
operator|=(Creation& left, Creation right) -> Creation&
{
  return left = left | right;
}

struct AllocationProperties
{
  Usage usage{ Usage::AUTO };
  Creation creation{ Creation::HOST_ACCESS_RANDOM_BIT };
};

class Allocator
{
public:
  explicit Allocator(const std::string& resource_name);

  template<typename T = void*>
  auto map_memory(VmaAllocation allocation) -> T*
  {
    T* data{ nullptr };
    vmaMapMemory(allocator, allocation, std::bit_cast<void**>(&data));
    return data;
  }

  void unmap_memory(VmaAllocation allocation);

  auto allocate_buffer(VkBuffer&,
                       VkBufferCreateInfo&,
                       const AllocationProperties&) -> VmaAllocation;
  auto allocate_buffer(VkBuffer&,
                       VmaAllocationInfo&,
                       VkBufferCreateInfo&,
                       const AllocationProperties&) -> VmaAllocation;
  auto allocate_image(VkImage&, VkImageCreateInfo&, const AllocationProperties&)
    -> VmaAllocation;
  auto allocate_image(VkImage&,
                      VmaAllocationInfo&,
                      VkImageCreateInfo&,
                      const AllocationProperties&) -> VmaAllocation;

  void deallocate_buffer(VmaAllocation, VkBuffer&);
  void deallocate_image(VmaAllocation, VkImage&);

  static auto get_allocator() -> VmaAllocator { return allocator; }

  static auto construct() -> void
  {
    allocator = construct_allocator(Device::the(), Instance::the());
    info("Created Allocator!");
  }

  static void destroy()
  {
    vmaDestroyAllocator(get_allocator());
    info("Destroyed Allocator!");
    allocator = nullptr;
  }

private:
  std::string resource_name{};

  static auto construct_allocator(const Device& device,
                                  const Instance& instance) -> VmaAllocator;
  static inline VmaAllocator allocator{ nullptr };
};

} // namespace Core
