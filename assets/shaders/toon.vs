#version 300 es

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
// layout(location = 3) in vec3 in_tangent;

uniform mat4 view_proj;
uniform mat4 model;

out vec3 vs_position;
out vec3 vs_normal;
out vec2 vs_texcoord;
// out mat3 vs_tbn;

void main()
{
  vs_position = in_position;
  vs_normal = transpose(inverse(mat3(model))) * in_normal;
  vs_texcoord = in_texcoord;
  gl_Position = view_proj * model * vec4(in_position, 1.0);
}
