#version 460

#include "buffers.glsl"
#include "util.glsl"

layout(location = 0) in vec3 fragment_normal;
layout(location = 1) in vec3 fragment_tangents;
layout(location = 2) in vec3 fragment_bitangents;
layout(location = 3) in vec2 fragment_uvs;
layout(location = 4) in vec3 world_space_fragment_position;
layout(location = 5) in vec4 shadow_space_fragment_position;
layout(location = 6) in vec4 colour;

layout(location = 0) out vec4 fragment_colour;

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
  fragment_colour = colour * vec4(mat_pc.albedo_colour, 1.0);
}

vec4
compute_normal_from_map(mat3 tbn)
{
  vec3 normal_map_value =
    normalize(texture(normal_map, fragment_uvs).rgb * 2.0 - vec3(1.0F));
  return vec4(normalize(tbn * normal_map_value), 1.0);
}
