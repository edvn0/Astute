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
  float cascade_splits[4];
}
renderer;

layout(std140, set = 0, binding = 1) uniform ShadowUBO
{
  mat4 view;
  mat4 projection;
  mat4 view_projection;
  vec4 sun_position;
  vec4 sun_direction;
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

layout(std140, set = 0, binding = 4) buffer VisiblePointLightSSBO
{
  int indices[MAX_LIGHT_COUNT];
}
visible_point_lights;

layout(std140, set = 0, binding = 5) buffer VisibleSpotLightSSBO
{
  int indices[MAX_LIGHT_COUNT];
}
visible_spot_lights;

layout(std140, set = 0, binding = 6) uniform ScreenDataUBO
{
  vec2 full_resolution;
  vec2 half_resolution;
  vec2 inv_resolution;
  vec2 depth_constants;
  float near_plane;
  float far_plane;
  float time;
  uint tile_count_x;
}
screen_data;

layout(std140, set = 0, binding = 7) uniform DirectionalShadowProjectionUBO
{
  mat4 view_projections[4];
}
directional_shadow_projections;

#endif
