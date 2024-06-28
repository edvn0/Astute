#include "pch/CorePCH.hpp"

#include "core/YAMLFile.hpp"

#include "logging/Logger.hpp"

namespace Engine::Core {

YAMLFile::YAMLFile() = default;

auto
YAMLFile::operator<<(const YAML::Node& new_node) -> YAMLFile&
{
  node.push_back(new_node);
  return *this;
}

auto
YAMLFile::write(const std::string_view path) -> bool
{
  std::filesystem::path output{ path };
  std::ofstream current{ output };
  if (!current) {
    error("Could not open path at: {}", path);
    return false;
  }

  current << node;
  return true;
}

} // namespace Engine::Core
