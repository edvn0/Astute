#include "pch/CorePCH.hpp"

#include "graphics/GPUBuffer.hpp"

#include "graphics/Allocator.hpp"

namespace Engine::Graphics {

static auto
to_string(GPUBuffer::Type buffer_type) -> std::string
{
  switch (buffer_type) {
    using enum Engine::Graphics::GPUBuffer::Type;
    case Vertex:
      return "Vertex";
    case Index:
      return "Index";
    case Storage:
      return "Storage";
    case Uniform:
      return "Uniform";
    default:
      return "Unknown";
  }
}

GPUBuffer::GPUBuffer(Type type, Core::usize input_size)
  : size(input_size)
  , buffer_type(type)
{
  construct_buffer();
}

GPUBuffer::~GPUBuffer()
{
  Allocator allocator{
    std::format("GPUBuffer::~GPUBuffer({}, {})", to_string(buffer_type), size),
  };
  allocator.deallocate_buffer(allocation, buffer);
}

auto
GPUBuffer::buffer_usage_flags() const -> VkBufferUsageFlags
{
  switch (buffer_type) {
    using enum Engine::Graphics::GPUBuffer::Type;
    case Vertex:
      return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    case Index:
      return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    case Storage:
      return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    case Uniform:
      return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
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

  VkBufferCreateInfo buffer_info{
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .size = size,
    .usage = buffer_usage_flags(),
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = nullptr,
  };

  const auto is_uniform = buffer_type == Type::Uniform;

  auto usage = is_uniform ? Usage::AUTO_PREFER_HOST : Usage::AUTO_PREFER_DEVICE;
  auto creation = Creation::MAPPED_BIT;
  if (is_uniform) {
    creation |= Creation::HOST_ACCESS_RANDOM_BIT;
  }

  allocation = allocator.allocate_buffer(buffer,
                                         allocation_info,
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

  if (this->allocation_info.pMappedData != nullptr) {
    std::memcpy(this->allocation_info.pMappedData, write_data, size);
  } else {
    auto* mapped = allocator.map_memory(this->allocation);
    std::memcpy(mapped, write_data, size);
    allocator.unmap_memory(this->allocation);
  }
}

} // namespace Engine::Graphic
