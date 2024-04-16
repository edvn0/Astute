#pragma once

#include "core/Types.hpp"

#include <span>
#include <type_traits>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace Engine::Graphics {

template<class T>
concept IsPOD = std::is_pod_v<T>;

template<IsPOD T>
class UniformBufferObject;

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

  template<class T, Core::usize Extent = std::dynamic_extent>
  auto write(std::span<T, Extent> data) -> void
  {
    write(data.data(), data.size_bytes());
  }

  template<class T, Core::usize Extent = std::dynamic_extent>
  auto write(std::span<const T, Extent> data) -> void
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

  template<IsPOD T>
  friend class UniformBufferObject;
};

class VertexBuffer
{
public:
  template<class VertexType, Core::usize Extent = std::dynamic_extent>
  VertexBuffer(std::span<VertexType, Extent> vertices)
    : buffer(GPUBuffer::Type::Vertex, vertices.size_bytes())
  {
    buffer.write(vertices);
  }

  template<class VertexType, Core::usize Extent = std::dynamic_extent>
  VertexBuffer(std::span<const VertexType, Extent> vertices)
    : buffer(GPUBuffer::Type::Vertex, vertices.size_bytes())
  {
    buffer.write(vertices);
  }

  template<class VertexType>
  VertexBuffer(std::span<VertexType> vertices)
    : buffer(GPUBuffer::Type::Vertex, vertices.size_bytes())
  {
    buffer.write(vertices);
  }

  template<class VertexType>
  VertexBuffer(std::span<const VertexType> vertices)
    : buffer(GPUBuffer::Type::Vertex, vertices.size_bytes())
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
    : buffer(GPUBuffer::Type::Index, indices.size_bytes())
  {
    buffer.write(indices);
  }

  explicit IndexBuffer(const std::span<Core::u32> indices)
    : buffer(GPUBuffer::Type::Index, indices.size_bytes())
  {
    buffer.write(indices);
  }

  auto size() const -> Core::usize { return buffer.get_size(); }
  auto count() const -> Core::usize
  {
    return buffer.get_size() / sizeof(Core::u32);
  }
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

template<IsPOD T>
class UniformBufferObject
{
public:
  explicit UniformBufferObject(const T& data)
  {
    buffer.write(&data, sizeof(T));
    descriptor_info = VkDescriptorBufferInfo{
      .buffer = buffer.get_buffer(),
      .offset = 0,
      .range = sizeof(T),
    };
  }

  explicit UniformBufferObject()
  {
    buffer.write(&pod_data, sizeof(T));
    descriptor_info = VkDescriptorBufferInfo{
      .buffer = buffer.get_buffer(),
      .offset = 0,
      .range = sizeof(T),
    };
  }

  auto size() const -> Core::usize { return buffer.get_size(); }
  auto get_buffer() const -> VkBuffer { return buffer.get_buffer(); }

  auto update(const T& data) -> void { buffer.write(&data, sizeof(T)); }
  auto update() -> void { buffer.write(&pod_data, sizeof(T)); }

  auto get_data() const -> const T& { return pod_data; }
  auto get_data() -> T& { return pod_data; }

  auto get_descriptor_info() const -> const VkDescriptorBufferInfo&
  {
    return descriptor_info;
  }

private:
  T pod_data;
  GPUBuffer buffer{ GPUBuffer::Type::Uniform, sizeof(T) };
  VkDescriptorBufferInfo descriptor_info{};
};

} // namespace Engine::Graphics
