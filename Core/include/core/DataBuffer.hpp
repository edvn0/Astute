#pragma once

#include <memory>
#include <span>
#include <vector>

#include "core/Exceptions.hpp"
#include "core/Types.hpp"

namespace Engine::Core {

auto
human_readable_size(usize bytes, i32 places = 2) -> std::string;

class WriteRangeException final : public AstuteBaseException
{
public:
  using AstuteBaseException::AstuteBaseException;
};

class DataBuffer
{
public:
  explicit DataBuffer(std::integral auto input_size)
    : buffer_size(static_cast<usize>(input_size))
  {
    // Zero is valid here!
  }
  DataBuffer(const u8* input_data, std::integral auto input_size)
    : buffer_size(static_cast<usize>(input_size))
  {
    if (input_size > 0) {
      allocate_storage(input_size);
      std::memcpy(data.get(), input_data, input_size);
    }
  }

  DataBuffer() = default;
  ~DataBuffer() = default;

  DataBuffer(DataBuffer&& other) noexcept
    : buffer_size(other.buffer_size)
    , data(std::move(other.data))
  {
    other.buffer_size = 0;
  }

  [[nodiscard]] auto raw() const -> const void* { return data.get(); }
  [[nodiscard]] auto span() const
  {
    return std::span{ data.get(), buffer_size };
  }

  [[nodiscard]] auto operator=(DataBuffer&& other) noexcept -> DataBuffer&
  {
    buffer_size = other.buffer_size;
    data = std::move(other.data);
    other.buffer_size = 0;
    return *this;
  }

  auto fill_zero() { fill_with(0); }

  template<typename T>
  auto write(const T* input_data, std::integral auto input_size) -> void
  {
    if (input_size > buffer_size) {
      throw WriteRangeException{ "DataBuffer::write: input_size > size" };
    }
    if (!data) {
      allocate_storage(input_size);
    }
    // Do i need to cast the input_data to u8*?
    std::memcpy(data.get(), input_data, input_size);
  }

  template<typename T>
  auto write(const T* input_data,
             std::integral auto input_size,
             std::integral auto offset) -> void
  {
    static_assert(std::is_unsigned_v<decltype(offset)>);

    // Check if input_size plus offset exceeds the buffer_size
    if (offset + input_size > buffer_size) {
      throw WriteRangeException{
        "DataBuffer::write: (offset + input_size) > buffer_size"
      };
    }

    if (input_size > buffer_size) {
      throw WriteRangeException{ "DataBuffer::write: input_size > size" };
    }
    if (!data) {
      allocate_storage(input_size);
    }

    std::memcpy(data.get() + offset, input_data, input_size);
  }

  template<typename T>
  auto write(const std::vector<T>& input_data, std::integral auto input_size)
    -> void
  {
    write(input_data.data(), input_size);
  }

  template<typename T>
  auto write(const std::vector<T>& input_data) -> void
  {
    write(input_data.data(), input_data.size() * sizeof(T));
  }

  template<typename T, std::size_t Extent = std::dynamic_extent>
  auto write(std::span<T, Extent> input_data) -> void
  {
    write(input_data.data(), input_data.size_bytes());
  }

  template<typename T, std::size_t Count>
  auto write(const std::array<T, Count>& input_data) -> void
  {
    write(input_data.data(), input_data.size() * sizeof(T));
  }

  template<typename T, std::size_t Extent = std::dynamic_extent>
  auto read(std::span<T, Extent> output, std::integral auto input_size) const
    -> void
  {
    if (input_size > buffer_size) {
      throw WriteRangeException{ "DataBuffer::read: input_size > size" };
    }
    if (!data) {
      throw WriteRangeException{ "DataBuffer::read: data is null" };
    }
    std::memcpy(output.data(), data.get(), input_size);
  }

  template<typename T, std::size_t Extent = std::dynamic_extent>
  auto read(std::span<T, Extent> output) const -> void
  {
    const auto size = output.size_bytes();
    if (size > buffer_size) {
      throw WriteRangeException{ "DataBuffer::read: input_size > size" };
    }
    if (!data) {
      throw WriteRangeException{ "DataBuffer::read: data is null" };
    }
    std::memcpy(output.data(), data.get(), size);
  }

  /***
   * @brief Read a vector of T from the databuffer
   * @tparam T The type of the vector
   * @param output The vector to write to
   * @param input_count The number of elements to read
   */
  template<typename T>
  auto read(std::vector<T>& output, std::integral auto input_count) const
    -> void
  {
    const auto actual_size = input_count * sizeof(T);
    // check vector size too
    if (output.size() < input_count) {
      throw WriteRangeException{
        "DataBuffer::read: input_count > vector size"
      };
    }
    if (actual_size > buffer_size) {
      throw WriteRangeException{ "DataBuffer::read: input_count > size" };
    }
    if (!data) {
      throw WriteRangeException{ "DataBuffer::read: data is null" };
    }
    // Do i need to cast the input_data to u8*?
    std::memcpy(output.data(), data.get(), actual_size);
  }

  template<typename T>
  auto read(std::vector<T>& output) const -> void
  {
    const auto actual_size = output.size() * sizeof(T);
    // check vector size too
    if (output.size() < output.size()) {
      throw WriteRangeException{
        "DataBuffer::read: output.size() > vector size"
      };
    }
    if (actual_size > buffer_size) {
      throw WriteRangeException{ "DataBuffer::read: output.size() > size" };
    }
    if (!data) {
      throw WriteRangeException{ "DataBuffer::read: data is null" };
    }
    // Do i need to cast the input_data to u8*?
    std::memcpy(output.data(), data.get(), actual_size);
  }

  /***
   * @brief Read an array of T from the databuffer
   * @tparam T The type of the vector
   * @tparam Count The number of elements to read
   * @param output The vector to write to
   */
  template<typename T, std::size_t Count>
  auto read(std::array<T, Count>& output) const -> void
  {
    if (sizeof(T) * Count > buffer_size) {
      throw WriteRangeException{ "DataBuffer::read: input_size > size" };
    }
    if (!data) {
      throw WriteRangeException{ "DataBuffer::read: data is null" };
    }
    // Do i need to cast the input_data to u8*?
    std::memcpy(output.data(), data.get(), sizeof(T) * Count);
  }

  auto copy_from(const DataBuffer& from)
  {
    buffer_size = from.size();
    std::memcpy(data.get(), from.data.get(), from.size());
  }

  auto clear() noexcept -> void
  {
    buffer_size = 0;
    data.reset();
  }

  [[nodiscard]] auto size() const noexcept -> usize { return buffer_size; }
  [[nodiscard]] auto hash() const noexcept -> usize
  {
    return std::hash<usize>{}(buffer_size) ^
           std::hash<std::unique_ptr<u8[]>>{}(data);
  }
  [[nodiscard]] auto valid() const noexcept -> bool
  {
    return data != nullptr && buffer_size > 0;
  }
  [[nodiscard]] operator bool() const noexcept { return valid(); }

  static auto empty() { return DataBuffer{ 0 }; }
  static auto copy(const DataBuffer& from) -> DataBuffer
  {
    DataBuffer constructed(from.size());
    constructed.allocate_storage(constructed.size());
    constructed.copy_from(from);
    return constructed;
  }

  auto set_size_and_reallocate(usize new_size) -> void
  {
    buffer_size = new_size;
    allocate_storage(new_size);
  }

private:
  usize buffer_size{ 0 };
  std::unique_ptr<u8[]> data{ nullptr };

  auto allocate_storage(std::integral auto new_size) -> void
  {
    allocate_storage(static_cast<usize>(new_size));
  }
  auto allocate_storage(usize new_size) -> void;

  auto fill_with(std::integral auto value) -> void
  {
    if (!data) {
      allocate_storage(size());
    }

    const auto value_to_fill = value;
    std::memset(data.get(), value_to_fill, size());
  }
};

} // namespace Engine::Core
