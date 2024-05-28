#version 460

#include "buffers.glsl"
#include "util.glsl"

layout(location = 0) in vec2 v_tex_coord;

layout(location = 0) out vec4 frag_color;

layout(set = 1, binding = 7) uniform sampler2D fullscreen_texture;

layout(push_constant) uniform PushConstants
{
  float aberration_offset;
}
uniforms;

void
main()
{
  vec2 red_offset = v_tex_coord + vec2(uniforms.aberration_offset, 0.0);
  vec2 green_offset = v_tex_coord;
  vec2 blue_offset = v_tex_coord - vec2(uniforms.aberration_offset, 0.0);

  vec3 color;
  color.r = texture(fullscreen_texture, red_offset).r;
  color.g = texture(fullscreen_texture, green_offset).g;
  color.b = texture(fullscreen_texture, blue_offset).b;

  frag_color = vec4(color, 1.0);
}