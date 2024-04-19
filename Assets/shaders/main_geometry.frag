#version 460

#include "buffers.glsl"

layout(location = 0) in vec3 fragment_normal;
layout(location = 1) in vec2 fragment_uv_input;
layout(location = 2) in vec3 world_space_fragment_position;
layout(location = 3) in vec4 in_fragment_colour;
layout(location = 4) in vec4 shadow_space_fragment_position;

layout(location = 0) out vec4 out_fragment_colour;
layout(location = 1) out vec4 fragment_normals;
layout(location = 2) out vec4 fragment_uvs;

layout(set = 0, binding = 10) uniform sampler2D shadow_map;

void
main()
{
  // Use normal and uvs to create a colour
  fragment_normals = vec4(fragment_normal, 1.0);
  fragment_uvs = vec4(fragment_uv_input, 0.0, 1.0);

  vec4 fragment_colour = in_fragment_colour;

  // Use renderer.camera_position and shadow.sun_position to generate some
  // simple lighting
  vec3 ambient = vec3(0.3);
  vec3 light_direction =
    normalize(shadow.sun_position - world_space_fragment_position);
  vec3 view_direction =
    normalize(renderer.camera_position - world_space_fragment_position);
  vec3 half_direction = normalize(light_direction + view_direction);

  float diffuse_factor = max(dot(fragment_normal, light_direction), 0.0);
  vec3 diffuse_color = vec3(1.0, 1.0, 1.0); // Set your desired diffuse color

  float specular_factor =
    pow(max(dot(fragment_normal, half_direction), 0.0), 32.0);
  vec3 specular_color = vec3(1.0, 1.0, 1.0); // Set your desired specular color

  vec3 direct_light =
    diffuse_factor * diffuse_color + specular_factor * specular_color;

  // Shadow mapping with shadow_map and the shadow_space_fragment_position
  // variable
  vec4 shadow_space_coords = shadow_space_fragment_position;
  shadow_space_coords.xyz /= shadow_space_coords.w;
  shadow_space_coords = shadow_space_coords * 0.5 + 0.5;

  float shadow_factor = 1.0;
  float closest_depth = texture(shadow_map, shadow_space_coords.xy).r;
  float current_depth = shadow_space_coords.z;
  if (current_depth > closest_depth) {
    shadow_factor = 0.2;
  }

  vec3 lighting = ambient + shadow_factor * direct_light;

  out_fragment_colour = vec4(lighting, fragment_colour.a);
}
