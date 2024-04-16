#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uvs;

layout(location = 0) out vec3 fragment_normal;
layout(location = 1) out vec2 fragment_uvs;

void
main()
{
  gl_Position = vec4(position, 1.0);
  fragment_normal = normal;
  fragment_uvs = uvs;
}