#version 460

layout(location = 0) in vec3 fragment_normal;
layout(location = 1) in vec2 fragment_uv_input;

layout(location = 0) out vec4 fragment_colour;
layout(location = 1) out vec4 fragment_normals;
layout(location = 2) out vec4 fragment_uvs;

void
main()
{
  // Use normal and uvs to create a colour
  fragment_normals = vec4(fragment_normal, 1.0);
  fragment_uvs = vec4(fragment_uv_input, 0.0, 1.0);

  fragment_colour = vec4(0.4, 0.4, 0.4, 1.0);
}
