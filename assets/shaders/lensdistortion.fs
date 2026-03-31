#version 410

out vec4 FragColor;

in vec2 vs_texcoord;

uniform sampler2D screen;
uniform float strength;

void main()
{
  vec2 uv = vs_texcoord - 0.5;
  float angle = atan(uv.x, uv.y);
  float dist = dot(uv, uv);
  vec2 distorted = 0.5 + vec2(sin(angle), cos(angle)) * sqrt(dist) * (1.0 - strength * dist);
  FragColor = texture(screen, distorted);
}
