#ifndef ASTUTE_BUFFER
#define ASTUTE_BUFFER

layout(std140, set = 0, binding = 0) uniform RendererUBO
{
  mat4 view;
  mat4 projection;
  mat4 view_projection;
  vec4 light_colour_intensity;
  vec4 specular_colour_intensity;
  vec3 camera_position;
}
renderer;

layout(std140, set = 0, binding = 1) uniform ShadowUBO
{
  mat4 view;
  mat4 projection;
  mat4 view_projection;
  vec3 sun_position;
}
shadow;

#endif