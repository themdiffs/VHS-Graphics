#version 410

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 3) in vec3 in_tangent;

uniform mat4 view_proj;
uniform mat4 model;

out vec3 vs_position;
out vec3 vs_normal;
out vec2 vs_texcoord;
out mat3 vs_tbn;

void main()
{
  vs_position = vec3(model * vec4(in_position, 1.0));

  mat3 normal_matrix = transpose(inverse(mat3(model)));
  vec3 T = normalize(normal_matrix * in_tangent);
  vec3 N = normalize(normal_matrix * in_normal);
  T = normalize(T - dot(T, N) * N);
  vec3 B = cross(N, T);

  vs_tbn = mat3(T, B, N);

  vs_normal = N;
  vs_texcoord = in_texcoord;
  gl_Position = view_proj * model * vec4(in_position, 1.0);
}
