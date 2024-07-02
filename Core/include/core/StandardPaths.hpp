#pragma once

#include <array>
#include <filesystem>
#include <format>
#include <string_view>

namespace Engine::Core {

namespace Detail {
using namespace std::string_view_literals;

template<class T>
concept PathLike = std::is_convertible_v<T, std::string_view> ||
                   std::is_convertible_v<T, const char*> || (requires(T t) {
                     {
                       t.size()
                     } -> std::convertible_to<std::size_t>;
                     {
                       t.data()
                     } -> std::convertible_to<const char*>;
                   }) || std::is_array_v<T>;

#ifndef ASTUTE_BASE_ASSET_PATH
static constexpr auto base_path = std::string_view{ "Assets" };
static inline auto base_path_as_fp = std::filesystem::path{ base_path };
#else
static constexpr auto base_path = std::string_view{ ASTUTE_BASE_ASSET_PATH };
static inline auto base_path_as_fp = std::filesystem::path{ base_path };
#endif

#define CREATE_ASSET_PATH(x)                                                   \
  inline auto x##_directory() -> std::filesystem::path                         \
  {                                                                            \
    return Detail::base_path_as_fp / #x;                                       \
  }                                                                            \
  inline auto x##_file(const Detail::PathLike auto& path)                      \
    -> std::filesystem::path                                                   \
  {                                                                            \
    return Detail::base_path_as_fp / #x / path;                                \
  }
}

CREATE_ASSET_PATH(pipelines)
CREATE_ASSET_PATH(images)
CREATE_ASSET_PATH(shaders)
CREATE_ASSET_PATH(meshes)

} // namespace Engine::Core
