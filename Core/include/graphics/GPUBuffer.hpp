#pragma once

#include "core/Types.hpp"

#include <span>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace Engine::Graphics {

class GPUBuffer
{
public:
  enum class Type : Core::u8
  {
    Invalid,
    Vertex,
    Index,
    Uniform,
    Storage,
  };

  GPUBuffer(Type, Core::usize);
  ~GPUBuffer();

  auto get_size() const -> Core::usize { return size; }

  template<class T>
  auto write(const std::span<T> data) -> void
  {
    write(data.data(), data.size_bytes());
  }

  template<class T>
  auto write(const std::vector<T>& data) -> void
  {
    write(data.data(), data.size() * sizeof(T));
  }

  auto get_buffer() const -> VkBuffer { return buffer; }

private:
  Core::usize size;

  Type buffer_type{ Type::Invalid };
  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo allocation_info;

  auto write(const void*, Core::usize) -> void;
  auto construct_buffer() -> void;
  auto buffer_usage_flags() const -> VkBufferUsageFlags;
};

class VertexBuffer
{
public:
  template<class VertexType>
  VertexBuffer(const std::span<VertexType> vertices)
    : buffer(GPUBuffer::Type::Vertex, vertices.size() * sizeof(VertexType))
  {
    buffer.write(vertices);
  }

  auto size() const -> Core::usize { return buffer.get_size(); }
  auto get_buffer() const -> VkBuffer { return buffer.get_buffer(); }

private:
  GPUBuffer buffer;
};

class IndexBuffer
{
public:
  explicit IndexBuffer(const std::span<const Core::u32> indices)
    : buffer(GPUBuffer::Type::Index, indices.size() * sizeof(Core::u32))
  {
    buffer.write(indices);
  }
  explicit IndexBuffer(const std::span<Core::u32> indices)
    : buffer(GPUBuffer::Type::Index, indices.size() * sizeof(Core::u32))
  {
    buffer.write(indices);
  }

  auto size() const -> Core::usize { return buffer.get_size(); }
  auto get_buffer() const -> VkBuffer { return buffer.get_buffer(); }

private:
  GPUBuffer buffer;
};

class StorageBuffer
{
public:
  template<class T>
  StorageBuffer(const std::span<T> data)
    : buffer(GPUBuffer::Type::Storage, data.size_bytes())
  {
    buffer.write(data);
  }

  auto size() const -> Core::usize { return buffer.get_size(); }
  auto get_buffer() const -> VkBuffer { return buffer.get_buffer(); }

private:
  GPUBuffer buffer;
};

class UniformBuffer
{
public:
  template<class T>
  UniformBuffer(const std::span<T> data)
    : buffer(GPUBuffer::Type::Uniform, data.size_bytes())
  {
    buffer.write(data);
  }

  auto size() const -> Core::usize { return buffer.get_size(); }
  auto get_buffer() const -> VkBuffer { return buffer.get_buffer(); }

private:
  GPUBuffer buffer;
};

} // namespace Engine::Graphics
