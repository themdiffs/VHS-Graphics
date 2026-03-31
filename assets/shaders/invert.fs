#version 410

out vec4 FragColor;

in vec2 vs_texcoord;

uniform sampler2D screen;

void main()
{
  vec3 color = vec3(1.0) - texture(screen, vs_texcoord).rgb;
  FragColor = vec4(color, 1.0);
}
