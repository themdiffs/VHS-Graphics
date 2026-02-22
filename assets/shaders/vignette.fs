#version 300 es
precision mediump float;

out vec4 FragColor;

in vec2 vs_texcoord;

uniform sampler2D screen;
uniform float intensity;

void main()
{
  vec3 color = texture(screen, vs_texcoord).rgb;
  vec2 uv = vs_texcoord * (1.0 - vs_texcoord.yx);
  float vig = uv.x * uv.y * 15.0;
  vig = pow(vig, intensity);
  FragColor = vec4(color * vig, 1.0);
}
