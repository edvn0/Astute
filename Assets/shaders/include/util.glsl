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

#endif // ASTUTE_UTILS