#pragma once

#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>

namespace Engine::Core {

template<class T>
concept IsBuiltin = std::is_fundamental_v<T>;

template<class T, class To>
concept CStrConvertibleTo = std::convertible_to<T, To> || requires(const T& t) {
  {
    t.c_str()
  } -> std::convertible_to<To>;
} || requires(const T& t) {
  {
    t.data()
  } -> std::convertible_to<To>;
} || requires(const T& t) {
  {
    t.c_str()
  } -> std::convertible_to<const char*>;
} || requires(const T& t) {
  {
    t.data()
  } -> std::convertible_to<const char*>;
};

template<class T>
concept StringLike =
  std::is_same_v<const char*, T> || std::is_same_v<std::string_view, T> ||
  std::is_same_v<std::string, T> || CStrConvertibleTo<T, const char*> ||
  CStrConvertibleTo<T, const wchar_t*>;

template<class T>
concept IsNumber = std::is_arithmetic_v<T>;

template<class T, class OutputType>
concept TypeDoesSupply = requires(const T& t) {
  {
    t.get_output()
  } -> std::same_as<OutputType>;
};

template<typename E>
concept IsEnum =
  std::is_enum_v<E> && (std::is_integral_v<std::underlying_type_t<E>> ||
                        std::is_enum_v<std::underlying_type_t<E>>);

} // namespace Engine::Core
