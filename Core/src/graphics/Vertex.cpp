#include "pch/CorePCH.hpp"

#include "graphics/Vertex.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <unordered_map>

namespace Engine::Graphics {

auto
generate_attributes() -> std::vector<VkVertexInputAttributeDescription>
{
  std::vector<VkVertexInputAttributeDescription> attributes(10);
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

  attributes[5].binding = 0;
  attributes[5].location = 5;
  attributes[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributes[5].offset = offsetof(Vertex, colour);

  // 6, 7, 8, 9 are for row_zero, row_one, row_two and instance_colour
  // respectively

  attributes[6].binding = 1;
  attributes[6].location = 6;
  attributes[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributes[6].offset = 0;

  attributes[7].binding = 1;
  attributes[7].location = 7;
  attributes[7].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributes[7].offset = sizeof(glm::vec4);

  attributes[8].binding = 1;
  attributes[8].location = 8;
  attributes[8].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributes[8].offset = 2 * sizeof(glm::vec4);

  attributes[9].binding = 1;
  attributes[9].location = 9;
  attributes[9].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributes[9].offset = 3 * sizeof(glm::vec4);

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
  hash = hash * 37 + std::hash<glm::vec4>{}(k.colour);
  return hash;
}
