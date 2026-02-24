#version 300 es

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

uniform mat4 view_proj;
uniform mat4 model;

out vec3 vs_position;
out vec3 vs_normal;
out vec2 vs_texcoord;
out vec4 vs_light_proj_pos;

void main()
{
  vs_position = vec3(model * vec4(in_position, 1.0f));
  vs_normal = transpose(inverse(mat3(model))) * in_normal;
  vs_texcoord = in_texcoord;

vs_light_proj_pos = vs_light_proj_pos * vec4(vs_position, 1.0);
  gl_Position = view_proj * vec4(vs_position, 1.0);
}
