#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <vulkan/vulkan.h>

#include <compare>
#include <tuple>

namespace Engine::Graphics {
struct Vertex;
}
template<>
struct std::hash<Engine::Graphics::Vertex>
{
  auto operator()(const Engine::Graphics::Vertex& k) const -> std::size_t;
};

namespace Engine::Graphics {

auto
generate_vertex_attributes() -> std::vector<VkVertexInputAttributeDescription>;
auto
generate_instance_attributes()
  -> std::vector<VkVertexInputAttributeDescription>;

struct Vertex
{
  glm::vec3 position;
  glm::vec2 uvs;
  glm::vec3 normals;
  glm::vec3 tangent;
  glm::vec3 bitangent;

  constexpr auto operator<=>(const Vertex& other) const -> std::partial_ordering
  {
    if (auto cmp = compare(position, other.position); cmp != 0)
      return cmp;
    if (auto cmp = compare(uvs, other.uvs); cmp != 0)
      return cmp;
    if (auto cmp = compare(normals, other.normals); cmp != 0)
      return cmp;
    if (auto cmp = compare(tangent, other.tangent); cmp != 0)
      return cmp;
    return compare(bitangent, other.bitangent);
  }

private:
  constexpr std::partial_ordering compare(const glm::vec3& a,
                                          const glm::vec3& b) const
  {
    if (auto cmp = a.x <=> b.x; cmp != 0)
      return cmp;
    if (auto cmp = a.y <=> b.y; cmp != 0)
      return cmp;
    return a.z <=> b.z;
  }

  // Comparison for glm::vec2
  constexpr std::partial_ordering compare(const glm::vec2& a,
                                          const glm::vec2& b) const
  {
    if (auto cmp = a.x <=> b.x; cmp != 0)
      return cmp;
    return a.y <=> b.y;
  }
};
}
