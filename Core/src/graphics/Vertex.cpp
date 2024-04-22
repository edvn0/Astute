#include "pch/CorePCH.hpp"

#include "graphics/Vertex.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <unordered_map>

namespace Engine::Graphics {

auto
generate_vertex_attributes() -> std::vector<VkVertexInputAttributeDescription>
{
  std::vector<VkVertexInputAttributeDescription> attributes(5);
  attributes[0].binding = 0;
  attributes[0].location = 0;
  attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributes[0].offset = offsetof(Vertex, position);

  attributes[1].binding = 0;
  attributes[1].location = 1;
  attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
  attributes[1].offset = offsetof(Vertex, uvs);

  attributes[2].binding = 0;
  attributes[2].location = 2;
  attributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributes[2].offset = offsetof(Vertex, normals);

  attributes[3].binding = 0;
  attributes[3].location = 3;
  attributes[3].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributes[3].offset = offsetof(Vertex, tangent);

  attributes[4].binding = 0;
  attributes[4].location = 4;
  attributes[4].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributes[4].offset = offsetof(Vertex, bitangent);

  return attributes;
}

auto
generate_instance_attributes() -> std::vector<VkVertexInputAttributeDescription>
{
  std::vector<VkVertexInputAttributeDescription> attributes(3);

  attributes[0].binding = 1;
  attributes[0].location = 5;
  attributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributes[0].offset = 0;

  attributes[1].binding = 1;
  attributes[1].location = 6;
  attributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributes[1].offset = sizeof(glm::vec4);

  attributes[2].binding = 1;
  attributes[2].location = 7;
  attributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributes[2].offset = 2 * sizeof(glm::vec4);

  return attributes;
}

} // namespace Engine::Graphics

auto
std::hash<Engine::Graphics::Vertex>::operator()(
  const Engine::Graphics::Vertex& k) const -> std::size_t
{
  auto hash = std::hash<glm::vec3>{}(k.position);
  hash = hash * 37 + std::hash<glm::vec2>{}(k.uvs);
  hash = hash * 37 + std::hash<glm::vec3>{}(k.normals);
  hash = hash * 37 + std::hash<glm::vec3>{}(k.tangent);
  hash = hash * 37 + std::hash<glm::vec3>{}(k.bitangent);
  return hash;
}
