#ifndef ASTUTE_UTILS
#define ASTUTE_UTILS

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

#endif // ASTUTE_UTILS
