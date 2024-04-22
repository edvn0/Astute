#version 460

#include "buffers.glsl"

layout(location = 0) in vec2 input_uvs;

layout(set = 0, binding = 10) uniform sampler2D gPosition;
layout(set = 0, binding = 11) uniform sampler2D gNormal;
layout(set = 0, binding = 12) uniform sampler2D gAlbedoSpec;

layout(location = 0) out vec4 FragColor;

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
  // retrieve data from G-buffer
  vec3 FragPos = texture(gPosition, input_uvs).rgb;
  vec3 Normal = texture(gNormal, input_uvs).rgb;
  vec4 AlbedoSpecular = texture(gAlbedoSpec, input_uvs);
  vec3 Albedo = AlbedoSpecular.rgb;
  float Specular = AlbedoSpecular.a;

  // then calculate lighting as usual
  vec3 lighting = Albedo * 0.1; // hard-coded ambient component

  vec3 viewDir = normalize(renderer.camera_position - FragPos);

  uint count_point_lights = point_lights.count;
  uint count_spot_lights = spot_lights.count;

  for (int i = 0; i < count_point_lights; ++i) {
    // calculate distance between light source and current fragment
    vec3 light_dir = point_lights.lights[i].pos - FragPos;
    float dist = length(light_dir);
    if (dist < point_lights.lights[i].radius) {
      lighting += calculate_diffuse_and_specular(
        Normal, FragPos, viewDir, point_lights.lights[i], Specular);
    }
  }

  for (int i = 0; i < count_spot_lights; ++i) {
    // calculate distance between light source and current fragment
    vec3 light_dir = spot_lights.lights[i].pos - FragPos;
    float dist = length(light_dir);
    if (dist < spot_lights.lights[i].range) {
      lighting += calculate_diffuse_and_specular(
        Normal, FragPos, viewDir, spot_lights.lights[i], Specular);
    }
  }

  // Apply tone mapping
  vec3 mapped = ACESTonemap(lighting);

  // Apply gamma correction
  vec3 gamma_corrected = pow(mapped, vec3(1.0 / 2.2));

  FragColor = vec4(gamma_corrected, 1.0);
}
