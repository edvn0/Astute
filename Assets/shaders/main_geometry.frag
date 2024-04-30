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

layout(set = 0, binding = 4) uniform sampler2D shadow_map;

layout(push_constant) uniform Material
{
  vec3 albedo_colour;
  float transparency;
  float roughness;
  float emission;
  uint use_normal_map;
}
mat_pc;

const vec3 ambient = vec3(0.15F);

float
projectShadow(vec4 shadowCoord, vec2 off)
{
  float shadow = 1.0;
  if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0) {
    float dist = texture(shadow_map, shadowCoord.st + off).r;
    if (shadowCoord.w > 0.0 && dist < shadowCoord.z) {
      shadow = ambient.x;
    }
  }
  return shadow;
}

float
filterPCF(vec4 sc)
{
  ivec2 texDim = textureSize(shadow_map, 0);
  float scale = 1.5;
  float dx = scale * 1.0 / float(texDim.x);
  float dy = scale * 1.0 / float(texDim.y);

  float shadowFactor = 0.0;
  int count = 0;
  int range = 1;

  for (int x = -range; x <= range; x++) {
    for (int y = -range; y <= range; y++) {
      shadowFactor += projectShadow(sc, vec2(dx * x, dy * y));
      count++;
    }
  }
  return shadowFactor / count;
}

void
main()
{
  // Use renderer.camera_position and shadow.sun_position to generate some
  // simple lighting
  vec3 N = fragment_normal;
  vec3 T = fragment_tangents;
  vec3 B = fragment_bitangents;
  mat3 TBN = mat3(T, B, N);
  vec3 tnorm = normalize(
    TBN * normalize(texture(normal_map, fragment_uvs).xyz * 2.0 - vec3(1.0)));
  fragment_normals =
    mat_pc.use_normal_map == 1 ? vec4(tnorm, 1.0) : vec4(N, 1.0);

  fragment_albedo_spec.rgb =
    mat_pc.albedo_colour * texture(albedo_map, fragment_uvs).xyz;
  fragment_albedo_spec.a = texture(specular_map, fragment_uvs).r;

  // Add directional light to fragment_albedo_spec
  vec3 as_vec3 = vec3(world_space_fragment_position);
  vec3 light_dir = normalize(shadow.sun_position - as_vec3);
  float diff = max(dot(N, light_dir), 0.0);
  vec3 diffuse = renderer.light_colour_intensity.xyz * diff *
                 renderer.light_colour_intensity.w;

  fragment_albedo_spec.rgb += diffuse;
  // Specular directional light
  vec3 reflect_dir = reflect(-light_dir, N);
  float spec = pow(
    max(dot(reflect_dir, normalize(renderer.camera_position - as_vec3)), 0.0),
    32.0);
  vec3 specular = renderer.specular_colour_intensity.xyz * spec *
                  renderer.specular_colour_intensity.w;

  fragment_albedo_spec.rgb += specular;

  float shadow_factor = filterPCF(shadow_space_fragment_position /
                                  shadow_space_fragment_position.w);
  // Add shadows
  // fragment_albedo_spec.rgb *= shadow_factor;
  // fragment_albedo_spec.a *= shadow_factor;

  // Add ambient to rgb
  fragment_albedo_spec.rgb += ambient;

  fragment_position = vec4(world_space_fragment_position, 1.0);
}
