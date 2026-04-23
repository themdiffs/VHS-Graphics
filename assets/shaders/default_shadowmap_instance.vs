#version 410

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 3) in mat4 in_instancedMatrix;

uniform mat4 view_proj;
uniform mat4 model; // TODO: remove
uniform mat4 light_view_proj;
uniform float snap_resolution;

out vec3 vs_position;
out vec3 vs_normal;
out vec2 vs_texcoord;
out vec4 vs_light_proj_pos;

vec4 vertexSnapping(vec4 clip_pos, float resolution)
{
  // perspective divide
  vec2 snap = clip_pos.xy / clip_pos.w;
  // grid snap
  snap = floor(snap * resolution * 0.5) / resolution;
  // undo perspective divide
  return vec4(snap * clip_pos.w, clip_pos.zw);
}

void main()
{
  vec4 worldPos = model * vec4(in_position, 1.0);

  vs_position = worldPos.xyz;
  vs_normal = transpose(inverse(mat3(model))) * in_normal;
  vs_texcoord = in_texcoord;

  vs_light_proj_pos = light_view_proj * worldPos;
  gl_Position = vertexSnapping(view_proj * worldPos, snap_resolution);
}
