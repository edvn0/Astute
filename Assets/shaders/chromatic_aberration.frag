#version 460

#include "buffers.glsl"
#include "util.glsl"

layout(location = 0) in vec2 v_tex_coord;

layout(location = 0) out vec4 final_fragment_colour;

layout(set = 1, binding = 7) uniform sampler2D fullscreen_texture;

layout(push_constant) uniform PushConstants
{
  vec3 aberration_offset;
}
uniforms;
vec3
tonemap_aces(vec3 color);

vec3
tonemap_aces(vec4 color);

void
main()
{
  vec2 red_offset = v_tex_coord + vec2(uniforms.aberration_offset.x, 0.0);
  vec2 green_offset = v_tex_coord + vec2(uniforms.aberration_offset.y, 0.0);
  vec2 blue_offset = v_tex_coord + vec2(uniforms.aberration_offset.z, 0.0);

  vec3 color;
  color.r = texture(fullscreen_texture, red_offset).r;
  color.g = texture(fullscreen_texture, green_offset).g;
  color.b = texture(fullscreen_texture, blue_offset).b;
  final_fragment_colour = vec4(color, 1.0);
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
