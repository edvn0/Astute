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
layout(location = 3) out vec4 fragment_shadow_position;

layout(set = 1, binding = 5) uniform sampler2D normal_map;
layout(set = 1, binding = 6) uniform sampler2D albedo_map;
layout(set = 1, binding = 7) uniform sampler2D specular_map;
layout(set = 1, binding = 8) uniform sampler2D roughness_map;

layout(push_constant) uniform Material
{
  vec3 albedo_colour;
  float transparency;
  float roughness;
  float emission;

  uint use_normal_map;
}
mat_pc;

vec4
compute_normal_from_map(mat3 tbn);

void
main()
{
  vec3 N = normalize(fragment_normal);
  vec3 T = normalize(fragment_tangents);
  vec3 B = normalize(fragment_bitangents);
  mat3 TBN = mat3(T, B, N);
  if (mat_pc.use_normal_map == 1) {
    fragment_normals = compute_normal_from_map(TBN);
  } else {
    fragment_normals = vec4(N, 0.0);
  }

  vec3 albedo_color =
    texture(albedo_map, fragment_uvs).rgb * mat_pc.albedo_colour;
  float specular_strength = texture(specular_map, fragment_uvs).r;
  float roughness_value =
    texture(roughness_map, fragment_uvs).r * mat_pc.roughness;

  fragment_albedo_spec.rgb =
    albedo_color + mat_pc.emission * mat_pc.albedo_colour;
  fragment_albedo_spec.a = specular_strength * roughness_value;

  fragment_position = vec4(world_space_fragment_position, 1.0);

  fragment_shadow_position = shadow_space_fragment_position;
}

vec4
compute_normal_from_map(mat3 tbn)
{
  vec3 normal_map_value =
    normalize(texture(normal_map, fragment_uvs).rgb * 2.0 - vec3(1.0F));
  return vec4(normalize(tbn * normal_map_value), 1.0);
}
