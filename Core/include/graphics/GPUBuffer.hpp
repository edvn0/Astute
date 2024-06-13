#pragma once

#include "core/DataBuffer.hpp"
#include "core/Types.hpp"

#include <span>
#include <vector>
#include <vulkan/vulkan.h>

namespace Engine::Graphics {

enum class GPUBufferType : Core::u8
{
  Invalid,
  Vertex,
  Index,
  Uniform,
  Storage,
  Staging,
};

template<class T, GPUBufferType BufferType>
class UniformBufferObject;

struct GPUBufferImpl;
class GPUBuffer
{
public:
  GPUBuffer(GPUBufferType, Core::usize);
  ~GPUBuffer();
  auto destroy() -> void;

  [[nodiscard]] constexpr auto get_size() const -> Core::usize { return size; }

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

  [[nodiscard]] auto get_buffer() const -> VkBuffer { return buffer; }
  auto copy_to(GPUBuffer&) -> void;

  [[nodiscard]] auto get_descriptor_info() const -> const auto&
  {
    return descriptor_info;
  }

private:
  Core::usize size;
  bool is_destroyed{ false };

  GPUBufferType buffer_type{ GPUBufferType::Invalid };
  VkBuffer buffer;
  Core::Scope<GPUBufferImpl> alloc_impl;

  VkDescriptorBufferInfo descriptor_info{};

  auto write(const void*, Core::usize) -> void;
  auto construct_buffer() -> void;
  [[nodiscard]] auto buffer_usage_flags() const -> VkBufferUsageFlags;

  template<class T, GPUBufferType BufferType>
  friend class UniformBufferObject;
  friend class VertexBuffer;
  friend class IndexBuffer;
  friend class StagingBuffer;
  friend class StorageBuffer;
};

class StagingBuffer
{
public:
  template<class T>
  explicit StagingBuffer(const std::span<T> data)
    : buffer(GPUBufferType::Staging, data.size_bytes())
  {
    buffer.write(data);
  }

  explicit StagingBuffer(Core::DataBuffer&& data)
    : StagingBuffer(data.span())
  {
  }

  auto destroy() { buffer.destroy(); }

  [[nodiscard]] constexpr auto size() const -> Core::usize
  {
    return buffer.get_size();
  }
  [[nodiscard]] auto get_buffer() const -> VkBuffer
  {
    return buffer.get_buffer();
  }

  [[nodiscard]] auto get_descriptor_info() const -> const auto&
  {
    return buffer.get_descriptor_info();
  }

private:
  GPUBuffer buffer;

  friend class VertexBuffer;
};

class VertexBuffer
{
public:
  template<class VertexType, Core::usize Extent = std::dynamic_extent>
  explicit VertexBuffer(std::span<VertexType, Extent> vertices)
    : buffer(GPUBufferType::Vertex, vertices.size_bytes())
  {
    auto temp = StagingBuffer{ vertices };
    temp.buffer.copy_to(buffer);
  }

  template<class VertexType, Core::usize Extent = std::dynamic_extent>
  explicit VertexBuffer(std::span<const VertexType, Extent> vertices)
    : buffer(GPUBufferType::Vertex, vertices.size_bytes())
  {
    auto temp = StagingBuffer{ vertices };
    temp.buffer.copy_to(buffer);
  }

  template<class VertexType>
  explicit VertexBuffer(std::span<VertexType> vertices)
    : buffer(GPUBufferType::Vertex, vertices.size_bytes())
  {
    auto temp = StagingBuffer{ vertices };
    temp.buffer.copy_to(buffer);
  }

  template<class VertexType>
  explicit VertexBuffer(std::span<const VertexType> vertices)
    : buffer(GPUBufferType::Vertex, vertices.size_bytes())
  {
    auto temp = StagingBuffer{ vertices };
    temp.buffer.copy_to(buffer);
  }

  explicit VertexBuffer(Core::usize new_size)
    : buffer(GPUBufferType::Vertex, new_size)
  {
    Core::DataBuffer temp_buffer{ new_size };
    temp_buffer.fill_zero();
    buffer.write(temp_buffer.raw(), new_size);
  }

  [[nodiscard]] auto size() const -> Core::usize { return buffer.get_size(); }
  [[nodiscard]] auto get_buffer() const -> VkBuffer
  {
    return buffer.get_buffer();
  }

  template<typename U>
  void write(std::span<U> data)
  {
    buffer.write(data.data(), data.size_bytes());
  }

  template<typename U>
  void write(const U* data, const Core::u32 size)
  {
    buffer.write(data, size);
  }

  [[nodiscard]] auto get_descriptor_info() const -> const auto&
  {
    return buffer.get_descriptor_info();
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

  explicit IndexBuffer(const void* indices, Core::usize size)
    : buffer(GPUBufferType::Index, size)
  {
    buffer.write(indices, size);
  }

  [[nodiscard]] auto size() const -> Core::usize { return buffer.get_size(); }
  [[nodiscard]] auto count() const -> Core::usize
  {
    return buffer.get_size() / sizeof(Core::u32);
  }
  [[nodiscard]] auto get_buffer() const -> VkBuffer
  {
    return buffer.get_buffer();
  }

  [[nodiscard]] auto get_descriptor_info() const -> const auto&
  {
    return buffer.get_descriptor_info();
  }

private:
  GPUBuffer buffer;
};

class StorageBuffer
{
public:
  template<class T>
  explicit StorageBuffer(const std::span<T> data)
    : buffer(GPUBufferType::Storage, data.size_bytes())
  {
    buffer.write(data);
  }

  explicit StorageBuffer(const Core::usize size)
    : buffer(GPUBufferType::Storage, size)
  {
  }

  [[nodiscard]] auto size() const -> Core::usize { return buffer.get_size(); }
  [[nodiscard]] auto get_buffer() const -> VkBuffer
  {
    return buffer.get_buffer();
  }

  [[nodiscard]] auto get_descriptor_info() const -> const auto&
  {
    return buffer.get_descriptor_info();
  }

  template<typename U>
  void write(std::span<U> data)
  {
    buffer.write(data.data(), data.size_bytes());
  }

private:
  GPUBuffer buffer;
};

class IShaderBindable
{
public:
  virtual ~IShaderBindable() = default;
  [[nodiscard]] virtual auto get_buffer() const -> VkBuffer = 0;
  [[nodiscard]] virtual auto get_descriptor_info() const
    -> const VkDescriptorBufferInfo& = 0;
  [[nodiscard]] virtual auto get_name() const -> const std::string& = 0;
  [[nodiscard]] virtual auto size() const -> Core::usize = 0;
};

template<class T, GPUBufferType BufferType = GPUBufferType::Uniform>
class UniformBufferObject : public IShaderBindable
{
public:
  explicit UniformBufferObject(const T& data,
                               const std::string_view input_identifier)
    : pod_data(data)
    , identifier(input_identifier)
  {
    buffer = Core::make_scope<GPUBuffer>(BufferType, sizeof(T));
    buffer->write(&pod_data, sizeof(T));
  }

  UniformBufferObject()
    : identifier(T::name)
  {
    buffer = Core::make_scope<GPUBuffer>(BufferType, sizeof(T));
    buffer->write(&pod_data, sizeof(T));
  }

  explicit UniformBufferObject(Core::usize size,
                               const std::string_view input_identifier)
    : identifier(input_identifier)
  {
    buffer = Core::make_scope<GPUBuffer>(BufferType, size);
    Core::DataBuffer zero{ size };
    zero.fill_zero();
    buffer->write(zero.raw(), size);
  }

  ~UniformBufferObject() override = default;

  auto resize(Core::usize new_size) -> void
  {
    buffer = Core::make_scope<GPUBuffer>(BufferType, new_size);
    Core::DataBuffer zero{ new_size };
    zero.fill_zero();
    buffer->write(zero.raw(), new_size);
  }

  [[nodiscard]] auto size() const -> Core::usize override
  {
    return buffer->get_size();
  }
  [[nodiscard]] auto get_buffer() const -> VkBuffer override
  {
    return buffer->get_buffer();
  }

  auto update(const T& data) -> void { buffer->write(&data, sizeof(T)); }
  auto update() -> void { buffer->write(&pod_data, sizeof(T)); }

  template<typename U>
  void write(std::span<U> data)
  {
    buffer->write(data.data(), data.size_bytes());
  }
  auto get_data() const -> const T& { return pod_data; }
  auto get_data() -> T& { return pod_data; }

  [[nodiscard]] auto get_descriptor_info() const
    -> const VkDescriptorBufferInfo& override
  {
    return buffer->get_descriptor_info();
  }

  [[nodiscard]] auto get_name() const -> const std::string& override
  {
    return identifier;
  }

private:
  T pod_data{};
  Core::Scope<GPUBuffer> buffer;
  std::string identifier;
};

} // namespace Engine::Graphics
