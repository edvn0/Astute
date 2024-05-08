#version 460

#include "buffers.glsl"
#include "util.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uvs;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangents;
layout(location = 4) in vec3 bitangents;

layout(location = 5) in vec4 transform_row_zero;
layout(location = 6) in vec4 transform_row_one;
layout(location = 7) in vec4 transform_row_two;

layout(location = 0) out vec3 fragment_normal;
layout(location = 1) out vec3 fragment_tangents;
layout(location = 2) out vec3 fragment_bitangents;
layout(location = 3) out vec2 fragment_uvs;
layout(location = 4) out vec3 world_space_fragment_position;
layout(location = 5) out vec4 shadow_space_fragment_position;

const mat4 bias = mat4(0.5,
                       0.0,
                       0.0,
                       0.0,
                       0.0,
                       0.5,
                       0.0,
                       0.0,
                       0.0,
                       0.0,
                       1.0,
                       0.0,
                       0.5,
                       0.5,
                       0.0,
                       1.0);

invariant precise gl_Position;

void
main()
{
  mat4 model = from_instance_to_model_matrix(
    transform_row_zero, transform_row_one, transform_row_two);
  vec4 computed = model * vec4(position, 1.0);
  gl_Position = renderer.view_projection * computed;

  mat3 local_normals = mat3(transpose(inverse(model)));
  fragment_normal = normalize(local_normals * normalize(normal));
  fragment_tangents = normalize(local_normals * normalize(tangents));
  fragment_bitangents = normalize(local_normals * normalize(bitangents));
  fragment_uvs = uvs;

  world_space_fragment_position = computed.xyz;
  shadow_space_fragment_position = shadow.view_projection * computed;
}
