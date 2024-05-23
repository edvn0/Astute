#ifndef ASTUTE_UTILS
#define ASTUTE_UTILS

#define TILE_SIZE 16
#define MAX_LIGHTS_PER_TILE 1024

mat4
from_instance_to_model_matrix(vec4 row_zero, vec4 row_one, vec4 row_two)
{
  return mat4(vec4(row_zero.x, row_one.x, row_two.x, 0.0),
              vec4(row_zero.y, row_one.y, row_two.y, 0.0),
              vec4(row_zero.z, row_one.z, row_two.z, 0.0),
              vec4(row_zero.w, row_one.w, row_two.w, 1.0));
}

float
random(vec2 st, float time)
{
  // Combine position with time to alter the seed
  return fract(sin(dot(st.xy, vec2(12.9898, 78.233)) + time) * 43758.5453123);
}

vec3
random_color(vec2 st, float time)
{
  // Generate three random numbers using slightly offset positions and time
  float r = random(st, time);
  float g = random(st + vec2(0.1), time);
  float b = random(st + vec2(0.2), time);
  return vec3(r, g, b);
}

uint
get_tile_index(vec2 frag_coord)
{
  ivec2 tile_id = ivec2(frag_coord) / ivec2(TILE_SIZE, TILE_SIZE);
  return tile_id.y * screen_data.tile_count_x + tile_id.x;
}

uint
get_visible_point_light_count(vec2 frag_coord)
{
  uint index = get_tile_index(frag_coord);
  uint offset = index * MAX_LIGHTS_PER_TILE;
  uint count = 0;
  while (count < MAX_LIGHTS_PER_TILE &&
         visible_point_lights.indices[offset + count] != -1) {
    count++;
  }
  return count;
}

PointLight
get_visible_point_light(vec2 frag_coord, uint i)
{
  uint index = get_tile_index(frag_coord);
  uint offset = index * MAX_LIGHTS_PER_TILE;
  int light_index = visible_point_lights.indices[offset + i];
  return point_lights.lights[light_index];
}

uint
get_visible_spot_light_count(vec2 frag_coord)
{
  uint index = get_tile_index(frag_coord);
  uint offset = index * MAX_LIGHTS_PER_TILE;
  uint count = 0;
  while (count < MAX_LIGHTS_PER_TILE &&
         visible_spot_lights.indices[offset + count] != -1) {
    count++;
  }
  return count;
}

SpotLight
get_visible_spot_light(vec2 frag_coord, uint i)
{
  uint index = get_tile_index(frag_coord);
  uint offset = index * MAX_LIGHTS_PER_TILE;
  int light_index = visible_spot_lights.indices[offset + i];
  return spot_lights.lights[light_index];
}

#endif // ASTUTE_UTILS
