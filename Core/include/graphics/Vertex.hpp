#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace Engine::Graphics {

auto
generate_attributes() -> std::vector<VkVertexInputAttributeDescription>;

struct Vertex
{
  glm::vec3 position;
  glm::vec2 uvs;
  glm::vec3 normals;
  glm::vec3 tangent;
  glm::vec3 bitangent;
  glm::vec4 color;
};

using Index = std::uint32_t;

}