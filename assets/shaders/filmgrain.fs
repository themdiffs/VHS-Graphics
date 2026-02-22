#version 300 es
precision mediump float;

out vec4 FragColor;

in vec2 vs_texcoord;

uniform sampler2D screen;
uniform float time;
uniform float strength;

void main()
{
  vec3 color = texture(screen, vs_texcoord).rgb;
  float noise = fract(sin(dot(vs_texcoord, vec2(12.9898, 78.233) * 2.0)) * 43758.5453 * (sin(time) * 0.5 + 1.0));
  color -= vec3(noise) * strength;
  FragColor = vec4(color, 1.0);
}
