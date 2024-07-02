#version 460

layout(location = 0) in vec3 inTexCoords;
layout(location = 0) out vec4 outColor;
layout(location = 1) out uint outCount;

layout(set = 1, binding = 15) uniform samplerCube cubemap;

void
main()
{
  vec3 coords = vec3(inTexCoords.x, 1.0F - inTexCoords.y, inTexCoords.z);
  outColor = texture(cubemap, inTexCoords);
  outCount = 0;
}
