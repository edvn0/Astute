#include "pch/CorePCH.hpp"

#include "core/DataBuffer.hpp"

#include "logging/Logger.hpp"

namespace Engine::Core {

auto
DataBuffer::allocate_storage(usize new_size) -> void
{
  if (data) {
    info("Resetting data storage at {}. Old size was: {}, new size is: {}",
         (void*)data.get(),
         human_readable_size(size()),
         human_readable_size(new_size));
    data.reset();
  }
  data = std::make_unique<u8[]>(new_size);
}

auto
human_readable_size(usize bytes, i32 places) -> std::string
{
  constexpr std::array<const char*, 5> units = {
    "Bytes", "KB", "MB", "GB", "TB"
  };
  const auto max_unit_index = units.size() - 1;

  usize unit_index = 0;
  auto size = static_cast<double>(bytes);

  while (size >= 1024 && unit_index < max_unit_index) {
    size /= 1024;
    ++unit_index;
  }

  return std::format("{:.{}f} {}", size, places, units[unit_index]);
}

} // namespace Engine::Core
