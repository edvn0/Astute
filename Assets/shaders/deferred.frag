#version 460

#include "buffers.glsl"
#include "util.glsl"

layout(location = 0) in vec2 input_uvs;

layout(set = 1, binding = 10) uniform sampler2D position_map;
layout(set = 1, binding = 11) uniform sampler2D normal_map;
layout(set = 1, binding = 12) uniform sampler2D albedo_specular_map;
layout(set = 1, binding = 13) uniform sampler2D shadow_position_map;
layout(set = 1, binding = 14) uniform sampler2D noise_map;

layout(constant_id = 0) const int NUM_SAMPLES = 4;

layout(location = 0) out vec4 final_fragment_colour;
layout(location = 1) out uint light_count;

const vec3 ambient = vec3(0.15F);

vec3
tonemap_aces(vec3 color);
vec3
tonemap_aces(vec4 color);

void
main()
{
  const float inverse_num_samples = 1.0F / float(NUM_SAMPLES);

  // Resolve G-buffer
  vec4 alb = texture(albedo_specular_map, input_uvs);
  vec3 position = texture(position_map, input_uvs).xyz;
  vec3 normal = normalize(texture(normal_map, input_uvs).xyz);
  float factor_if_in_shadow = texture(shadow_position_map, input_uvs).r;

  vec3 frag_colour = vec3(0.0);

  // Apply ambient light
  frag_colour += ambient + factor_if_in_shadow * alb.rgb;

  // Apply directional light
  vec3 L = -normalize(shadow.sun_direction.xyz);
  float diff = max(dot(normal, L), 0.0);
  vec3 main_diffuse = diff * renderer.light_colour_intensity.xyz *
                      renderer.light_colour_intensity.a;

  vec3 V = normalize(renderer.camera_position - position);
  vec3 R = reflect(-L, normal);

  float spec = pow(max(dot(V, R), 0.0), 32);
  vec3 main_specular = spec * renderer.specular_colour_intensity.xyz *
                       renderer.specular_colour_intensity.a;

  frag_colour += factor_if_in_shadow * (main_diffuse + main_specular);

  uint visible_point_light_count = get_visible_point_light_count(input_uvs);
  for (uint i = 0; i < visible_point_light_count; ++i) {
    PointLight light = get_visible_point_light(input_uvs, i);

    vec3 L = normalize(light.pos - position);
    float dist = length(light.pos - position);
    float attenuation = 1.0 / (1.0 + light.falloff * dist * dist);

    float diff = max(dot(normal, L), 0.0);
    vec3 diffuse = diff * light.radiance * light.intensity * attenuation;

    vec3 R = reflect(-L, normal);
    float spec = pow(max(dot(V, R), 0.0), 32);
    vec3 specular = spec * light.radiance * light.intensity * attenuation;

    frag_colour += factor_if_in_shadow * (diffuse + specular);
  }

  // Spot lights
  uint visible_spot_light_count = get_visible_spot_light_count(input_uvs);
  for (uint i = 0; i < visible_spot_light_count; ++i) {
    SpotLight light = get_visible_spot_light(input_uvs, i);

    vec3 L = normalize(light.pos - position);
    float dist = length(light.pos - position);
    float attenuation = 1.0 / (1.0 + light.falloff * dist * dist);

    float theta = dot(L, normalize(light.direction));
    float epsilon = light.angle - light.angle_attenuation;
    float intensity =
      clamp((theta - epsilon) / (light.angle - epsilon), 0.0, 1.0);

    float diff = max(dot(normal, L), 0.0);
    vec3 diffuse =
      diff * light.radiance * light.intensity * intensity * attenuation;

    vec3 R = reflect(-L, normal);
    float spec = pow(max(dot(V, R), 0.0), 32);
    vec3 specular =
      spec * light.radiance * light.intensity * intensity * attenuation;

    frag_colour += factor_if_in_shadow * (diffuse + specular);
  }

  final_fragment_colour = vec4(frag_colour, 1.0);
  light_count = visible_point_light_count + visible_spot_light_count;
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
