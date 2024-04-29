#version 460

#include "buffers.glsl"

layout(location = 0) in vec2 input_uvs;

layout(set = 1, binding = 10) uniform sampler2DMS gPositionMap;
layout(set = 1, binding = 11) uniform sampler2DMS gNormalMap;
layout(set = 1, binding = 12) uniform sampler2DMS gAlbedoSpecMap;

layout(constant_id = 0) const int NUM_SAMPLES = 4;

layout(location = 0) out vec4 FragColor;

vec4
resolve(sampler2DMS tex, ivec2 uv)
{
  vec4 result = vec4(0.0);
  for (int i = 0; i < NUM_SAMPLES; i++) {
    vec4 val = texelFetch(tex, uv, i);
    result += val;
  }
  // Average resolved samples
  return result / float(NUM_SAMPLES);
}

float
calculate_attenuation(float dist, PointLight light)
{
  float attenuation = 0.0;
  float range = light.radius - light.min_radius;
  if (range > 0.0) {
    float factor = clamp((dist - light.min_radius) / range, 0.0, 1.0);
    attenuation = 1.0 - pow(factor, light.falloff);
  }
  return attenuation * light.multiplier;
}

float
calculate_attenuation(float dist, SpotLight light)
{
  float attenuation = 0.0;
  float range = light.range;
  if (range > 0.0) {
    float factor = clamp(dist / range, 0.0, 1.0);
    attenuation = 1.0 - pow(factor, light.falloff);
  }
  return attenuation * light.multiplier;
}

vec3
calculate_diffuse_and_specular(const in vec3 normal,
                               const in vec3 fragPos,
                               const in vec3 viewDir,
                               const in PointLight light,
                               const in float specular_strength)
{
  vec3 lightDir = normalize(light.pos - fragPos);
  float dist = length(light.pos - fragPos);
  vec3 reflectDir = reflect(-lightDir, normal);

  float diff = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = diff * light.radiance * light.radiance;

  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0F);
  vec3 specular = spec * specular_strength * light.radiance;

  float attenuation = calculate_attenuation(dist, light);

  // float shadow = light.casts_shadows ? compute_shadow(fragPos, lightDir)
  // : 1.0;
  float shadow = light.casts_shadows ? 0.1 : 1.0;

  vec3 result = (diffuse + specular) * attenuation * shadow;
  return result;
}

vec3
calculate_diffuse_and_specular(const in vec3 normal,
                               const in vec3 fragPos,
                               const in vec3 viewDir,
                               const in SpotLight light,
                               const in float specular_strength)
{
  vec3 lightDir = normalize(light.pos - fragPos);
  float dist = length(light.pos - fragPos);
  vec3 reflectDir = reflect(-lightDir, normal);

  float diff = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = diff * light.radiance * light.radiance;

  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0F);
  vec3 specular = spec * specular_strength * light.radiance;

  float attenuation = calculate_attenuation(dist, light);

  // float shadow = light.casts_shadows ? compute_shadow(fragPos, lightDir)
  // : 1.0;
  float shadow = light.casts_shadows ? 0.1 : 1.0;

  vec3 result = (diffuse + specular) * attenuation * shadow;
  return result;
}

vec3
calculateLighting(vec3 pos,
                  vec3 normal,
                  vec4 albedo,
                  uint point_light_count,
                  uint spot_light_count)
{
  vec3 result = vec3(0.0);

  for (int i = 0; i < point_light_count; ++i) {
    // Vector to light
    vec3 L = point_lights.lights[i].pos.xyz - pos;
    // Distance from light to fragment position
    float dist = length(L);

    // Viewer to fragment
    vec3 V = renderer.camera_position.xyz - pos;
    V = normalize(V);

    // Light to fragment
    L = normalize(L);

    // Attenuation
    float atten = point_lights.lights[i].radius / (pow(dist, 2.0) + 1.0);

    // Diffuse part
    vec3 N = normalize(normal);
    float NdotL = max(0.0, dot(N, L));
    vec3 diff = point_lights.lights[i].radiance * albedo.rgb * NdotL * atten;

    // Specular part
    vec3 R = reflect(-L, N);
    float NdotR = max(0.0, dot(R, V));
    vec3 spec =
      point_lights.lights[i].radiance * albedo.a * pow(NdotR, 8.0) * atten;

    result += diff + spec;
  }

  for (int i = 0; i < spot_light_count; ++i) {
    // Vector to light
    vec3 L = spot_lights.lights[i].pos - pos;
    // Distance from light to fragment position
    float dist = length(L);
    // Normalize L
    L = normalize(L);

    // Check if fragment is within the light cone
    float spotEffect = dot(-L, normalize(spot_lights.lights[i].direction));
    if (spotEffect > cos(radians(spot_lights.lights[i].angle))) {
      // Attenuation including distance and cone attenuation
      float atten = (1.0 / (pow(dist, 2.0) + 1.0)) *
                    pow(spotEffect, spot_lights.lights[i].falloff);

      // Viewer to fragment
      vec3 V = normalize(renderer.camera_position - pos);

      // Diffuse part
      vec3 N = normalize(normal);
      float NdotL = max(0.0, dot(N, L));
      vec3 diff = spot_lights.lights[i].radiance * albedo.rgb * NdotL * atten;

      // Specular part
      vec3 R = reflect(-L, N);
      float NdotR = max(0.0, dot(R, V));
      vec3 spec =
        spot_lights.lights[i].radiance * albedo.a * pow(NdotR, 8.0) * atten;

      result += diff + spec;
    }
  }
  return result;
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
ACESTonemap(vec3 color)
{
  vec3 v = aces_m1 * color;
  vec3 a = v * (v + 0.0245786) - 0.000090537;
  vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
  return clamp(aces_m2 * (a / b), 0.0, 1.0);
}

void
main()
{
  ivec2 attDim = textureSize(gPositionMap);
  ivec2 UV = ivec2(input_uvs * attDim);
  uint count_point_lights = point_lights.count;
  uint count_spot_lights = spot_lights.count;

  // Ambient part
  vec4 alb = resolve(gAlbedoSpecMap, UV);
  vec3 fragColor = vec3(0.0);

  // Calualte lighting for every MSAA sample
  for (int i = 0; i < NUM_SAMPLES; i++) {
    vec3 pos = texelFetch(gPositionMap, UV, i).rgb;
    vec3 normal = texelFetch(gNormalMap, UV, i).rgb;
    vec4 albedo = texelFetch(gAlbedoSpecMap, UV, i);
    fragColor += calculateLighting(
      pos, normal, albedo, count_point_lights, count_spot_lights);
  }

  fragColor = (alb.rgb * 0.15) + fragColor / float(NUM_SAMPLES);

  vec3 mapped = ACESTonemap(fragColor);
  vec3 gamma_corrected = pow(mapped, vec3(1.0 / 2.2));
  FragColor = vec4(gamma_corrected, 1.0);
}
