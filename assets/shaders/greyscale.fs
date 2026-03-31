#version 410

out vec4 FragColor;

in vec2 vs_texcoord;

uniform sampler2D screen;

void main()
{
  vec3 color = texture(screen, vs_texcoord).rgb;
  float gray = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
  FragColor = vec4(vec3(gray), 1.0);
}
