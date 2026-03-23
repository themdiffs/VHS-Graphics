#version 300 es

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 3) in mat4 in_instancedMatrix;

uniform mat4 view_proj;
uniform mat4 model; // TODO: remove
uniform mat4 light_view_proj;

out vec3 vs_position;
out vec3 vs_normal;
out vec2 vs_texcoord;
out vec4 vs_light_proj_pos;

void main()
{
  vs_position = vec3(in_instancedMatrix * vec4(in_position, 1.0));
  vs_normal = transpose(inverse(mat3(in_instancedMatrix))) * in_normal;
  vs_texcoord = in_texcoord;

  vs_light_proj_pos = light_view_proj * vec4(vs_position, 1.0);
  gl_Position = view_proj * vec4(vs_position, 1.0);
}
