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

void
calculate_light_for(vec3 L,
                    vec3 N,
                    vec3 V,
                    vec4 albedo,
                    float atten,
                    vec3 radiance,
                    out vec3 result)
{
  // Combine calculations into fewer lines to reduce complexity
  vec3 R = reflect(-L, N);
  float NdotL = max(dot(N, L), 0.0);
  float NdotR = max(dot(R, V), 0.0);
  result += radiance * albedo.rgb * NdotL * atten;
  result += radiance * albedo.a * pow(NdotR, 8.0) * atten;
}

vec3
calculateLighting(vec3 pos,
                  vec3 normal,
                  vec4 albedo,
                  uint point_light_count,
                  uint spot_light_count)
{
  vec3 result = vec3(0.0);
  vec3 N = normalize(normal);                         // Normalizing once
  vec3 V = normalize(renderer.camera_position - pos); // Compute once, use many

  // Precompute constants for attenuation formula
  const float linearTerm = 0.09;
  const float quadraticTerm = 0.032;

  for (uint i = 0; i < point_light_count; ++i) {
    vec3 L = point_lights.lights[i].pos - pos;
    float dist = length(L);
    if (dist > point_lights.lights[i].radius) {
      continue;
    }

    L = normalize(L);
    float atten = 1.0 / (1.0 + linearTerm * dist + quadraticTerm * dist * dist);

    calculate_light_for(
      L, N, V, albedo, atten, point_lights.lights[i].radiance, result);
  }

  for (uint i = 0; i < spot_light_count; ++i) {
    vec3 L = spot_lights.lights[i].pos - pos;
    float dist = length(L);
    if (dist > spot_lights.lights[i].range) {
      continue;
    }

    L = normalize(L);
    float spot_effect = dot(-L, normalize(spot_lights.lights[i].direction));
    if (spot_effect <= cos(radians(spot_lights.lights[i].angle + 10.0))) {
      continue;
    }

    float atten = (1.0 / (dist * dist + 1.0)) *
                  pow(spot_effect, spot_lights.lights[i].falloff);

    calculate_light_for(
      L, N, V, albedo, atten, spot_lights.lights[i].radiance, result);
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
