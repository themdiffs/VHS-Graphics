#version 300 es
precision mediump float;

out vec4 FragColor;

in vec2 vs_texcoord;

uniform sampler2D screen;
uniform float offset;

void main()
{
  vec3 color;
  color.r = texture(screen, vs_texcoord + vec2(offset, 0.0)).r;
  color.g = texture(screen, vs_texcoord).g;
  color.b = texture(screen, vs_texcoord - vec2(offset, 0.0)).b;
  FragColor = vec4(color, 1.0);
}
