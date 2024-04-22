#version 460

#include "buffers.glsl"

layout(location = 0) in vec3 fragment_normal;
layout(location = 1) in vec3 fragment_tangents;
layout(location = 2) in vec3 fragment_bitangents;
layout(location = 3) in vec2 fragment_uvs;
layout(location = 4) in vec3 world_space_fragment_position;
layout(location = 5) in vec4 shadow_space_fragment_position;

layout(location = 0) out vec4 fragment_position;
layout(location = 1) out vec4 fragment_normals;
layout(location = 2) out vec4 fragment_albedo_spec;

layout(set = 0, binding = 10) uniform sampler2DShadow shadow_map;
layout(set = 0, binding = 5) uniform sampler2D normal_map;

void
main()
{

  // Use renderer.camera_position and shadow.sun_position to generate some
  // simple lighting
  vec3 ambient = vec3(0.3);
  vec3 N = normalize(fragment_normal);
  vec3 T = normalize(fragment_tangents);
  vec3 B = normalize(fragment_bitangents);
  mat3 TBN = mat3(T, B, N);
  vec3 tnorm =
    normalize(TBN * texture(normal_map, fragment_uvs).xyz * 2.0 - vec3(1.0));
  fragment_normals = vec4(tnorm, 1.0);
  fragment_albedo_spec = vec4(ambient, 1.0);
  fragment_position = vec4(world_space_fragment_position, 1.0);
}
