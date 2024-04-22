#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <vulkan/vulkan.h>

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

  constexpr auto operator<=>(const Vertex&) const = default;
};

using Index = std::uint32_t;

}
