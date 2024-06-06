#include "pch/CorePCH.hpp"

#include "graphics/GPUBuffer.hpp"

#include "graphics/Allocator.hpp"

#include "logging/Logger.hpp"

#include <vk_mem_alloc.h>

namespace Engine::Graphics {

static auto
to_string(GPUBufferType buffer_type) -> std::string
{
  switch (buffer_type) {
    using enum Engine::Graphics::GPUBufferType;
    case Vertex:
      return "Vertex";
    case Index:
      return "Index";
    case Storage:
      return "Storage";
    case Uniform:
      return "Uniform";
    case Staging:
      return "Staging";
    default:
      return "Unknown";
  }
}

struct GPUBufferImpl
{
  VmaAllocation allocation{};
  VmaAllocationInfo allocation_info{};
};

GPUBuffer::GPUBuffer(GPUBufferType type, Core::usize input_size)
  : size(input_size)
  , buffer_type(type)
{
  alloc_impl = Core::make_scope<GPUBufferImpl>();
  construct_buffer();

  descriptor_info = {
    .buffer = buffer,
    .offset = 0,
    .range = get_size(),
  };
}

GPUBuffer::~GPUBuffer()
{
  if (!is_destroyed) {
    destroy();
  }
}

auto
GPUBuffer::destroy() -> void
{
  if (is_destroyed) {
    return;
  }

  trace("Destroying buffer of type: {}, size: {}",
        to_string(buffer_type),
        Core::human_readable_size(size));

  Allocator allocator{
    std::format("GPUBuffer::~GPUBuffer({}, {})", to_string(buffer_type), size),
  };
  allocator.deallocate_buffer(alloc_impl->allocation, buffer);

  is_destroyed = true;
}

auto
GPUBuffer::buffer_usage_flags() const -> VkBufferUsageFlags
{
  switch (buffer_type) {
    using enum Engine::Graphics::GPUBufferType;
    case Vertex:
      return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
             VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    case Index:
      return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    case Storage:
      return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    case Uniform:
      return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    case Staging:
      return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    default:
      return 0;
  }
}

auto
GPUBuffer::construct_buffer() -> void
{
  Allocator allocator{
    std::format(
      "GPUBuffer::construct_buffer({}, {})", to_string(buffer_type), size),
  };

  trace("Creating buffer of type: {}, size: {}",
        to_string(buffer_type),
        Core::human_readable_size(size));

  std::array family_indices{
    Device::the().get_family(QueueType::Graphics),
    Device::the().get_family(QueueType::Transfer),
  };
  VkBufferCreateInfo buffer_info{
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .size = size,
    .usage = buffer_usage_flags(),
    .sharingMode = VK_SHARING_MODE_CONCURRENT,
    .queueFamilyIndexCount = static_cast<Core::u32>(family_indices.size()),
    .pQueueFamilyIndices = family_indices.data(),
  };

  auto usage = (buffer_type == GPUBufferType::Staging)
                 ? Usage::AUTO_PREFER_HOST
                 : Usage::AUTO_PREFER_DEVICE;
  auto creation = Creation::MAPPED_BIT | Creation::HOST_ACCESS_RANDOM_BIT;

  alloc_impl->allocation =
    allocator.allocate_buffer(buffer,
                              alloc_impl->allocation_info,
                              buffer_info,
                              {
                                .usage = usage,
                                .creation = creation,
                              });
}

auto
GPUBuffer::write(const void* write_data, const Core::usize write_size) -> void
{
  Allocator allocator{
    std::format("GPUBuffer::write({}, {})", to_string(buffer_type), size),
  };
  if (write_size > size) {
    throw std::runtime_error("Data size is larger than buffer size");
  }

  if (alloc_impl->allocation_info.pMappedData != nullptr) {
    std::memcpy(
      alloc_impl->allocation_info.pMappedData, write_data, write_size);
  } else {
    auto* mapped =
      std::bit_cast<void*>(allocator.map_memory(alloc_impl->allocation));
    std::memcpy(mapped, write_data, write_size);
    allocator.unmap_memory(alloc_impl->allocation);
  }
}

auto
GPUBuffer::copy_to(GPUBuffer& dest) -> void
{
  Device::the().execute_immediate(QueueType::Transfer, [&](auto* cmd) {
    VkBufferCopy copy_region = {};
    copy_region.size = size;
    vkCmdCopyBuffer(cmd, buffer, dest.get_buffer(), 1, &copy_region);
  });
}

} // namespace Engine::Graphic
