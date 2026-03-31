#version 410

out vec4 FragColor;

in vec2 vs_texcoord;

uniform sampler2D screen;
uniform float gamma;

void main()
{
  vec3 color = texture(screen, vs_texcoord).rgb;
  color = pow(color, vec3(1.0 / gamma));
  FragColor = vec4(color, 1.0);
}
