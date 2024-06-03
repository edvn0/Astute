#version 460

#include "buffers.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 colour;

layout(location = 0) out flat vec4 out_colour;
layout(location = 1) out vec4 out_position;

invariant precise gl_Position;

void
main()
{
  vec4 computed = vec4(position, 1.0);
  gl_Position = renderer.view_projection * computed;
  out_position = computed;
  out_colour = colour;
}
