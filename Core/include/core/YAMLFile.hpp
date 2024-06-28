#pragma once

#include "core/Types.hpp"

#include <filesystem>

#include <yaml-cpp/yaml.h>

namespace Engine::Core {

class YAMLFile
{
public:
  YAMLFile();
  auto load(const std::filesystem::path&) -> bool;

  template<class T>
  auto operator[](const std::string& key) -> std::optional<const T>
  {
    if (!node[key].IsDefined()) {
      return std::nullopt;
    }

    return node[key].as<T>();
  }

  template<class T>
  auto get_or_throw(const std::string& key)
  {
    return node[key].as<T>();
  }

  auto write(std::string_view) -> bool;

  auto is_valid() { return valid_file && node.IsDefined(); }

  auto operator<<(const YAML::Node&) -> YAMLFile&;

private:
  YAML::Node node;
  bool valid_file{ false };
};

}
