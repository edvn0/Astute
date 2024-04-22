#version 460

#include "buffers.glsl"
#include "util.glsl"

layout(location = 0) in vec3 position;

layout(location = 5) in vec4 transform_row_zero;
layout(location = 6) in vec4 transform_row_one;
layout(location = 7) in vec4 transform_row_two;

invariant precise gl_Position;

void
main()
{
  mat4 model = from_instance_to_model_matrix(
    transform_row_zero, transform_row_one, transform_row_two);
  vec4 computed = model * vec4(position, 1.0);
  gl_Position = renderer.view_projection * computed;
}
