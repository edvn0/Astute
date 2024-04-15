#include "pch/CorePCH.hpp"

#include "graphics/GPUBuffer.hpp"

#include "graphics/Allocator.hpp"

namespace Engine::Graphics {

static auto
to_string(BufferType buffer_type) -> std::string
{
  switch (buffer_type) {
    using enum Engine::Graphics::BufferType;
    case Vertex:
      return "Vertex";
    case Index:
      return "Index";
    case Storage:
      return "Storage";
    default:
      return "Unknown";
  }
}

auto
GPUBuffer::write(const void* write_data, const Core::usize write_size) -> void
{
  Allocator allocator{ std::format("GPUBuffer::write({})",
                                   to_string(buffer_type)) };
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
