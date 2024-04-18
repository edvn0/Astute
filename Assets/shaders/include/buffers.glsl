#ifndef ASTUTE_BUFFER
#define ASTUTE_BUFFER

layout(std140, set = 0, binding = 0) uniform RendererUBO
{
  mat4 view;
  mat4 projection;
  mat4 view_projection;
  vec3 camera_position;
}
renderer;

layout(std140, set = 0, binding = 1) uniform ShadowUBO
{
  mat4 view;
  mat4 projection;
  mat4 view_projection;
  vec3 camera_position;
}
shadow;

#endif