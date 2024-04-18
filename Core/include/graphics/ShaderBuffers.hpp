#pragma once

#include <glm/glm.hpp>

namespace Engine::Graphics {

struct RendererUBO
{
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 view_proj;
  glm::vec3 camera_pos;
};

struct ShadowUBO
{
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 view_proj;
  glm::vec3 light_pos;
};

} // namespace Engine::Graphics
