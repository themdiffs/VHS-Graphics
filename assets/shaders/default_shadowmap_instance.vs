#version 410

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 3) in mat4 in_instancedMatrix;

uniform mat4 view_proj;
uniform mat4 model; // TODO: remove
uniform mat4 light_view_proj;
uniform vec2 screen_resolution;

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

vec4 vertexSnapping(vec4 position, vec2 resolution)
{
  vec3 perspective_divide = position.xyz / vec3(position.w);

  vec2 screen_coords = (perspective_divide.xy + vec2(1.0, 1.0)) * vec2(resolution.x, resolution.y) * 0.5;

  // snap by truncating to int
  vec2 screen_coords_truncated = vec2(int(screen_coords.x), int(screen_coords.y));

  vec2 clipRange = ((screen_coords_truncated * vec2(2.0, 2.0) / vec2(resolution.x, resolution.y)) - vec2(1.0, 1.0));

  vec4 pos = vec4(clipRange.x, clipRange.y, perspective_divide.z, position.w);
  // undo perspective divide
  pos.xyz *= position.w;

  return pos;
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
  
  gl_Position = vertexSnapping(view_proj * worldPos, screen_resolution);
}
