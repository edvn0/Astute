#pragma once

#include "core/Types.hpp"

#include <span>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace Engine::Graphics {

enum class BufferType : Core::u8
{
  Vertex,
  Index,
  Storage
};

class GPUBuffer
{
public:
  GPUBuffer(Core::usize input_size)
    : size(input_size)
  {
    construct_buffer();
  }

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

private:
  Core::usize size;

  BufferType buffer_type{ BufferType::Vertex };
  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo allocation_info;

  auto write(const void*, Core::usize) -> void;
  auto construct_buffer() -> void;
};

class VertexBuffer
{
public:
  template<class VertexType>
  VertexBuffer(const std::span<VertexType> vertices)
    : buffer(vertices.size() * sizeof(VertexType))
  {
    buffer.write(vertices);
  }

private:
  GPUBuffer buffer;
};

class IndexBuffer
{};

class StorageBuffer
{};

} // namespace Engine::Graphics
