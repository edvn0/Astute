#version 460

#include "buffers.glsl"

layout(location = 0) in vec2 input_uvs;

layout(set = 1, binding = 9) uniform sampler2D shadow_map;
layout(set = 1, binding = 10) uniform sampler2D position_map;
layout(set = 1, binding = 11) uniform sampler2D normal_map;
layout(set = 1, binding = 12) uniform sampler2D albedo_specular_map;
layout(set = 1, binding = 13) uniform sampler2D shadow_position_map;
layout(set = 1, binding = 14) uniform sampler2D noise_map;

layout(constant_id = 0) const int NUM_SAMPLES = 4;

layout(location = 0) out vec4 final_fragment_colour;

const vec3 ambient = vec3(0.15F);

vec3 tonemap_aces(vec3);

vec3 tonemap_aces(vec4);

void
main()
{
  const float inverse_num_samples = 1.0F / float(NUM_SAMPLES);
  uint count_point_lights = point_lights.count;
  uint count_spot_lights = spot_lights.count;

  // Resolve G-buffer
  vec4 alb = texture(albedo_specular_map, input_uvs);

  vec3 frag_colour = vec3(0.0);

  vec3 position = texture(position_map, input_uvs).xyz;
  vec3 normal = texture(normal_map, input_uvs).xyz;

  vec3 L = -shadow.sun_direction.xyz;
  float diff = max(dot(normal, L), 0.0);
  vec3 diffuse = diff * renderer.light_colour_intensity.xyz *
                 renderer.light_colour_intensity.a;

  vec3 V = normalize(renderer.camera_position - position);
  vec3 R = reflect(-L, normal);

  float spec = pow(max(dot(V, R), 0.0), 32);
  vec3 specular = spec * renderer.specular_colour_intensity.xyz *
                  renderer.specular_colour_intensity.a;

  vec3 mapped = tonemap_aces(alb.xyz + diffuse + specular);
  vec3 gamma_corrected = pow(mapped, vec3(1.0 / 2.2));
  final_fragment_colour = vec4(mapped, 1.0);
}

const mat3 aces_m1 = mat3(0.59719,
                          0.07600,
                          0.02840,
                          0.35458,
                          0.90834,
                          0.13383,
                          0.04823,
                          0.01566,
                          0.83777);
const mat3 aces_m2 = mat3(1.60475,
                          -0.10208,
                          -0.00327,
                          -0.53108,
                          1.10813,
                          -0.07276,
                          -0.07367,
                          -0.00605,
                          1.07602);

vec3
tonemap_aces(vec3 color)
{
  vec3 v = aces_m1 * color;
  vec3 a = v * (v + 0.0245786) - 0.000090537;
  vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
  return clamp(aces_m2 * (a / b), 0.0, 1.0);
}

vec3
tonemap_aces(vec4 color)
{
  vec3 v = aces_m1 * color.xyz;
  vec3 a = v * (v + 0.0245786) - 0.000090537;
  vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
  return clamp(aces_m2 * (a / b), 0.0, 1.0);
}
