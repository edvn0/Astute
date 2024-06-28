#pragma once

#include "core/Types.hpp"

#include <istream>
#include <ostream>

#include "logging/Logger.hpp"

namespace Engine::Core {

#define ASTUTE_MAKE_SERIALISABLE(T)                                            \
  static auto write(const T&, std::ostream&) -> bool;                          \
  static auto read(T&, const std::istream&) -> bool;                           \
  static auto construct_file_path(const T&) -> std::string;

template<class T>
concept SerialWriteable = requires(T& input_type,
                                   const T& output_type,
                                   const std::istream& input_stream,
                                   std::ostream& output_stream) {
  {
    T::write(output_type, output_stream)
  } -> std::same_as<bool>;
  {
    T::read(input_type, input_stream)
  } -> std::same_as<bool>;
  {
    T::construct_file_path(output_type)
  } -> std::same_as<std::string>;
};

class SerialWriter
{
public:
  template<SerialWriteable T>
  static auto write(const T& instance) -> bool
  {
    auto path = T::construct_file_path(instance);
    std::ofstream output_file{ path };
    output_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    if (!output_file) {
      return false;
    }

    try {
      return T::write(instance, output_file);
    } catch (const std::exception& exc) {
      error(exc);
      return false;
    }
  }

  template<SerialWriteable T>
  static auto read(const std::istream& input_file, T& instance) -> bool
  {
    if (!input_file) {
      return false;
    }

    try {
      return T::read(input_file, instance);
    } catch (const std::exception& exc) {
      error(exc);
      return false;
    }
  }
};

} // namespace Engine::Core
