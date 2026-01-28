#version 300 es

precision mediump float;

out vec4 FragColor;

in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_texcoord;

uniform vec3 light_direction;
uniform vec3 camera_position;

void main()
{
  vec3 normal = normalize(vs_normal);

  // diffuse
  vec3 toLight = -light_direction;
  float diffuseFactor = max(dot(normal, toLight), 0.0);
  vec3 diffuseColor = diffuseFactor * vec3(1.0);

  FragColor = vec4(diffuseColor, 1.0);
}
