#version 460

#include "buffers.glsl"
#include "util.glsl"

layout(location = 0) in vec3 position;

layout(location = 5) in vec4 transform_row_zero;
layout(location = 6) in vec4 transform_row_one;
layout(location = 7) in vec4 transform_row_two;

// Push constant with a single u32 value (cascadeIndex)

layout(push_constant) uniform PushConstant
{
  uint cascade_index;
}
pc;

invariant precise gl_Position;

void
main()
{
  mat4 model = from_instance_to_model_matrix(
    transform_row_zero, transform_row_one, transform_row_two);
  vec4 computed = model * vec4(position, 1.0);
  gl_Position =
    directional_shadow_projections.view_projections[pc.cascade_index] *
    computed;
}
