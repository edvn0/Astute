#pragma once

#include "core/DataBuffer.hpp"
#include "core/Types.hpp"

#include <span>
#include <type_traits>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace Engine::Graphics {

enum class GPUBufferType : Core::u8
{
  Invalid,
  Vertex,
  Index,
  Uniform,
  Storage,
};

template<class T, GPUBufferType BufferType>
class UniformBufferObject;

class GPUBuffer
{
public:
  GPUBuffer(GPUBufferType, Core::usize);
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

  GPUBufferType buffer_type{ GPUBufferType::Invalid };
  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo allocation_info;

  auto write(const void*, Core::usize) -> void;
  auto construct_buffer() -> void;
  auto buffer_usage_flags() const -> VkBufferUsageFlags;

  template<class T, GPUBufferType BufferType>
  friend class UniformBufferObject;
  friend class VertexBuffer;
};

class VertexBuffer
{
public:
  template<class VertexType, Core::usize Extent = std::dynamic_extent>
  explicit VertexBuffer(std::span<VertexType, Extent> vertices)
    : buffer(GPUBufferType::Vertex, vertices.size_bytes())
  {
    buffer.write(vertices);
  }

  template<class VertexType, Core::usize Extent = std::dynamic_extent>
  explicit VertexBuffer(std::span<const VertexType, Extent> vertices)
    : buffer(GPUBufferType::Vertex, vertices.size_bytes())
  {
    buffer.write(vertices);
  }

  template<class VertexType>
  explicit VertexBuffer(std::span<VertexType> vertices)
    : buffer(GPUBufferType::Vertex, vertices.size_bytes())
  {
    buffer.write(vertices);
  }

  template<class VertexType>
  explicit VertexBuffer(std::span<const VertexType> vertices)
    : buffer(GPUBufferType::Vertex, vertices.size_bytes())
  {
    buffer.write(vertices);
  }

  explicit VertexBuffer(Core::usize new_size)
    : buffer(GPUBufferType::Vertex, new_size)
  {
    Core::DataBuffer temp_buffer{ new_size };
    temp_buffer.fill_zero();
    buffer.write(temp_buffer.raw(), new_size);
  }

  auto size() const -> Core::usize { return buffer.get_size(); }
  auto get_buffer() const -> VkBuffer { return buffer.get_buffer(); }

  template<typename U>
  void write(std::span<U> data)
  {
    buffer.write(data.data(), data.size_bytes());
  }

private:
  GPUBuffer buffer;
};

class IndexBuffer
{
public:
  explicit IndexBuffer(const std::span<const Core::u32> indices)
    : buffer(GPUBufferType::Index, indices.size_bytes())
  {
    buffer.write(indices);
  }

  explicit IndexBuffer(const std::span<Core::u32> indices)
    : buffer(GPUBufferType::Index, indices.size_bytes())
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
    : buffer(GPUBufferType::Storage, data.size_bytes())
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
  explicit UniformBuffer(const std::span<T> data)
    : buffer(GPUBufferType::Uniform, data.size_bytes())
  {
    buffer.write(data);
  }

  auto size() const -> Core::usize { return buffer.get_size(); }
  auto get_buffer() const -> VkBuffer { return buffer.get_buffer(); }

private:
  GPUBuffer buffer;
};

class IShaderBindable
{
public:
  virtual ~IShaderBindable() = default;
  virtual auto get_buffer() const -> VkBuffer = 0;
  virtual auto get_descriptor_info() const -> const VkDescriptorBufferInfo& = 0;
  virtual auto get_name() const -> const std::string& = 0;
  virtual auto size() const -> Core::usize = 0;
};

template<class T, GPUBufferType BufferType = GPUBufferType::Uniform>
class UniformBufferObject : public IShaderBindable
{
public:
  explicit UniformBufferObject(const T& data,
                               const std::string_view input_identifier)
    : identifier(input_identifier)
  {
    buffer = Core::make_scope<GPUBuffer>(BufferType, sizeof(T));
    buffer->write(&pod_data, sizeof(T));
    descriptor_info = VkDescriptorBufferInfo{
      .buffer = buffer->get_buffer(),
      .offset = 0,
      .range = sizeof(T),
    };
  }

  UniformBufferObject()
    : identifier(T::name)
  {
    buffer = Core::make_scope<GPUBuffer>(BufferType, sizeof(T));
    buffer->write(&pod_data, sizeof(T));
    descriptor_info = VkDescriptorBufferInfo{
      .buffer = buffer->get_buffer(),
      .offset = 0,
      .range = sizeof(T),
    };
  }

  explicit UniformBufferObject(Core::usize size,
                               const std::string_view input_identifier)
    : identifier(input_identifier)
  {
    buffer = Core::make_scope<GPUBuffer>(BufferType, size);
    Core::DataBuffer zero{ size };
    zero.fill_zero();
    buffer->write(zero.raw(), size);
    descriptor_info = VkDescriptorBufferInfo{
      .buffer = buffer->get_buffer(),
      .offset = 0,
      .range = size,
    };
  }

  ~UniformBufferObject() override = default;

  auto size() const -> Core::usize override { return buffer->get_size(); }
  auto get_buffer() const -> VkBuffer override { return buffer->get_buffer(); }

  auto update(const T& data) -> void { buffer->write(&data, sizeof(T)); }
  auto update() -> void { buffer->write(&pod_data, sizeof(T)); }

  template<typename U>
  void write(std::span<U> data)
  {
    buffer->write(data.data(), data.size_bytes());
  }
  auto get_data() const -> const T& { return pod_data; }
  auto get_data() -> T& { return pod_data; }

  auto get_descriptor_info() const -> const VkDescriptorBufferInfo& override
  {
    return descriptor_info;
  }

  auto get_name() const -> const std::string& override { return identifier; }

private:
  T pod_data{};
  Core::Scope<GPUBuffer> buffer;
  VkDescriptorBufferInfo descriptor_info{};
  std::string identifier;
};

} // namespace Engine::Graphics
