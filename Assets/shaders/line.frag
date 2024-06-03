
#version 460

layout(location = 0) in flat vec4 in_colour;
layout(location = 1) in vec4 in_position;

layout(location = 0) out vec4 fragment_position;
layout(location = 1) out vec4 fragment_normals;
layout(location = 2) out vec4 fragment_albedo_spec;
layout(location = 3) out float fragment_shadow_value;

void
main()
{
  fragment_albedo_spec = in_colour;
  fragment_position = in_position;
  fragment_shadow_value = 0.0F;
  fragment_normals = vec4(0, 1, 0, 0);
}
