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

layout(set = 1, binding = 5) uniform sampler2D normal_map;
layout(set = 1, binding = 6) uniform sampler2D albedo_map;
layout(set = 1, binding = 7) uniform sampler2D specular_map;
layout(set = 1, binding = 8) uniform sampler2D roughness_map;

layout(set = 0, binding = 4) uniform sampler2DShadow shadow_map;

layout(push_constant) uniform Material
{
  vec3 albedo_colour;
  float transparency;
  float roughness;
  float emission;
  bool use_normal_map;
}
mat_pc;

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

  float shadow = texture(shadow_map,
                         shadow_space_fragment_position.xyz /
                           shadow_space_fragment_position.w)
                   .r;

  fragment_normals = mat_pc.use_normal_map ? vec4(tnorm, 1.0) : vec4(N, 1.0);
  fragment_albedo_spec.rgb =
    shadow * mat_pc.albedo_colour * texture(albedo_map, fragment_uvs).xyz;
  fragment_albedo_spec.a = shadow * texture(roughness_map, fragment_uvs).r *
                           texture(specular_map, fragment_uvs).r;
  fragment_position = vec4(world_space_fragment_position, 1.0);
}
