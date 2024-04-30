#ifndef ASTUTE_BUFFER
#define ASTUTE_BUFFER

#define MAX_LIGHT_COUNT 1000

layout(std140, set = 0, binding = 0) uniform RendererUBO
{
  mat4 view;
  mat4 projection;
  mat4 view_projection;
  vec4 light_colour_intensity;
  vec4 specular_colour_intensity;
  vec3 camera_position;
}
renderer;

layout(std140, set = 0, binding = 1) uniform ShadowUBO
{
  mat4 view;
  mat4 projection;
  mat4 view_projection;
  vec3 sun_position;
}
shadow;

struct PointLight
{
  vec3 pos;
  float intensity;
  vec3 radiance;
  float min_radius;
  float radius;
  float falloff;
  float light_size;
  bool casts_shadows;
};
layout(std140, set = 0, binding = 2) uniform PointLightUBO
{
  uint count;
  PointLight lights[MAX_LIGHT_COUNT];
}
point_lights;

struct SpotLight
{
  vec3 pos;
  float intensity;
  vec3 direction;
  float angle_attenuation;
  vec3 radiance;
  float range;
  float angle;
  float falloff;
  bool soft_shadows;
  bool casts_shadows;
};
layout(std140, set = 0, binding = 3) uniform SpotLightUBO
{
  uint count;
  SpotLight lights[MAX_LIGHT_COUNT];
}
spot_lights;

#endif
