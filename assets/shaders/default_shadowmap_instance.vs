#version 410
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 3) in mat4 in_instancedMatrix;
uniform mat4 view_proj;
uniform mat4 model; // TODO: remove
uniform mat4 light_view_proj;
uniform float snap_resolution;
// Gouraud lighting
uniform vec3 camera_position;
uniform vec3 light_position;
uniform vec3 light_color;
uniform float shininess;
out vec3 vs_position;
out vec3 vs_normal;
out vec2 vs_texcoord;
out vec4 vs_light_proj_pos;
// Gouraud lighting
out vec3 vs_color;
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
  
  // Gouraud lighting
  vec3 normal = normalize(vs_normal);
  vec3 light_dir = normalize(light_position - vs_position);
  vec3 view_dir =  normalize(camera_position - vs_position);
  vec3 halfway_dir = normalize(light_dir + view_dir);
  
  float diff = max(dot(normal, light_dir), 0.0);
  float spec = pow(max(dot(normal, halfway_dir), 0.0), shininess);
  
  // vs_color = 0.1 * vec3(1.0) + diff * vec3(1.0) + spec * vec3(1.0);
  // vs_color = 0.1 * vec3(1.0) + diff * light_color + spec * light_color;
  vs_color = 0.1 * vec3(1.0) + diff * light_color + (spec * 0.3) * light_color;
  
  gl_Position = vertexSnapping(view_proj * worldPos, snap_resolution);
}
