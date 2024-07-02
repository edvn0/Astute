#version 460

#include "buffers.glsl"
#include "util.glsl"

layout(location = 0) out vec3 outTexCoords;

const vec3 positions[8] = vec3[8](vec3(-1.0, -1.0, -1.0),
                                  vec3(1.0, -1.0, -1.0),
                                  vec3(-1.0, 1.0, -1.0),
                                  vec3(1.0, 1.0, -1.0),
                                  vec3(-1.0, -1.0, 1.0),
                                  vec3(1.0, -1.0, 1.0),
                                  vec3(-1.0, 1.0, 1.0),
                                  vec3(1.0, 1.0, 1.0));

const int indices[36] = int[36](0,
                                1,
                                2,
                                2,
                                1,
                                3,
                                4,
                                0,
                                6,
                                6,
                                0,
                                2,
                                5,
                                4,
                                7,
                                7,
                                4,
                                6,
                                1,
                                5,
                                3,
                                3,
                                5,
                                7,
                                6,
                                2,
                                7,
                                7,
                                2,
                                3,
                                4,
                                5,
                                0,
                                0,
                                5,
                                1);

void
main()
{
  vec3 pos = positions[indices[gl_VertexIndex]];

  // Remove translation from the view matrix
  mat4 viewNoTranslation = mat4(mat3(renderer.view));

  vec4 clipPos = renderer.projection * viewNoTranslation * vec4(pos, 1.0);

  // Ensure the skybox is always at the far plane
  gl_Position = clipPos.xyww;

  outTexCoords = pos;
}
