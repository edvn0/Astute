#version 460

#include "buffers.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uvs;

layout(location = 0) out vec3 fragment_normal;
layout(location = 1) out vec2 fragment_uvs;

void
main()
{
  mat4 view_proj = renderer.view_projection;
  vec4 computed = view_proj * vec4(position, 1.0);
  gl_Position = computed;
  fragment_normal = normal;
  fragment_uvs = uvs;
}