#version 460

#extension GL_EXT_debug_printf : enable

#include "buffers.glsl"
#include "util.glsl"

layout(location = 0) in vec3 fragment_normal;
layout(location = 1) in vec3 fragment_tangents;
layout(location = 2) in vec3 fragment_bitangents;
layout(location = 3) in vec2 fragment_uvs;
layout(location = 4) in vec3 world_space_fragment_position;
layout(location = 5) in vec3 view_position;

layout(location = 0) out vec4 fragment_position;
layout(location = 1) out vec4 fragment_normals;
layout(location = 2) out vec4 fragment_albedo_spec;
layout(location = 3) out float fragment_shadow_value;

layout(set = 1, binding = 5) uniform sampler2D normal_map;
layout(set = 1, binding = 6) uniform sampler2D albedo_map;
layout(set = 1, binding = 7) uniform sampler2D specular_map;
layout(set = 1, binding = 8) uniform sampler2D roughness_map;
layout(set = 1, binding = 9) uniform sampler2DArray shadow_map; // 4 cascades!

layout(push_constant) uniform Material
{
  vec3 albedo_colour;
  float transparency;
  float roughness;
  float emission;

  uint use_normal_map;
}
mat_pc;

float project_texture(vec4, vec2, uint);

vec4
compute_normal_from_map(mat3 tbn);

const float epsilon = 0.00005;

void
main()
{
  if (abs(mat_pc.transparency) < epsilon) {
    discard;
  }

  vec3 N = normalize(fragment_normal);
  vec3 T = normalize(fragment_tangents);
  vec3 B = normalize(fragment_bitangents);
  mat3 TBN = mat3(T, B, N);
  if (mat_pc.use_normal_map == 1) {
    fragment_normals = compute_normal_from_map(TBN);
  } else {
    fragment_normals = vec4(N, 0.0);
  }

  vec4 sampled_albedo = texture(albedo_map, fragment_uvs);
  vec3 albedo_color = sampled_albedo.rgb * mat_pc.albedo_colour;

  float specular_strength = texture(specular_map, fragment_uvs).r;
  float roughness_value =
    texture(roughness_map, fragment_uvs).r * mat_pc.roughness;

  fragment_position = vec4(world_space_fragment_position, 1.0);
  uint chosen_cascade_index = 0;
  for (uint i = 0; i < 3; ++i) {
    if (view_position.z < renderer.cascade_splits[i]) {
      chosen_cascade_index = i + 1;
    }
  }
  vec3 cascade_colour = vec3(1.0);
  switch (chosen_cascade_index) {
    case 0:
      cascade_colour *= vec3(0);
      break;
    case 1:
      cascade_colour *= vec3(0.25f, 1.0f, 0.25f);
      break;
    case 2:
      cascade_colour *= vec3(0.25f, 0.25f, 1.0f);
      break;
    case 3:
      cascade_colour *= vec3(1.0F, 0.25f, 0.25f);
      break;
  }

  fragment_albedo_spec.rgb = albedo_color * cascade_colour;
  fragment_albedo_spec.a = specular_strength * roughness_value;

  vec4 shadow_space_fragment_position =
    bias *
    directional_shadow_projections.view_projections[chosen_cascade_index] *
    fragment_position;
  fragment_shadow_value = project_texture(shadow_space_fragment_position /
                                            shadow_space_fragment_position.w,
                                          vec2(0.0),
                                          chosen_cascade_index);
}

vec4
compute_normal_from_map(mat3 tbn)
{
  vec3 normal_map_value =
    normalize(texture(normal_map, fragment_uvs).rgb * 2.0 - vec3(1.0F));
  return vec4(normalize(tbn * normal_map_value), 1.0);
}

const float ambient = 0.15;

float
project_texture(vec4 shadowCoord, vec2 offset, uint cascade_index)
{
  float shadow = 1.0;
  float bias = 0.005;

  if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0) {
    float dist =
      texture(shadow_map, vec3(shadowCoord.st + offset, cascade_index)).r;
    if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
      shadow = ambient;
    }
  }
  return shadow;
}
