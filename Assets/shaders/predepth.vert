#version 460

#include "buffers.glsl"
#include "util.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uvs;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;
layout(location = 5) in vec4 color;

layout(location = 6) in vec4 transform_row_zero;
layout(location = 7) in vec4 transform_row_one;
layout(location = 8) in vec4 transform_row_two;
layout(location = 9) in vec4 instance_colour;

void
main()
{
  mat4 model = from_instance_to_model_matrix(
    transform_row_zero, transform_row_one, transform_row_two);
  mat4 view_proj = renderer.view_projection;
  vec4 computed = model * vec4(position, 1.0);
  gl_Position = view_proj * computed;
}
