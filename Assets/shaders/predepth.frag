#version 460

layout(location = 0) in vec3 fragment_normal;
layout(location = 1) in vec2 fragment_uvs;

layout(location = 0) out vec4 fragment_colour;

void
main()
{
  // Use normal and uvs to create a colour
  vec3 colour = fragment_normal;
  fragment_colour = vec4(colour, 1.0);
}