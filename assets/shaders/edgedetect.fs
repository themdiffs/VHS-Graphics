#version 300 es
precision mediump float;

out vec4 FragColor;

in vec2 vs_texcoord;

uniform sampler2D screen;

const float offset = 1.0 / 300.0;

const vec2 offsets[9] = vec2[](
  vec2(-offset, offset),
  vec2(0.0, offset),
  vec2(offset, offset),
  vec2(-offset, 0.0),
  vec2(0.0, 0.0),
  vec2(offset, 0.0),
  vec2(-offset, -offset),
  vec2(0.0, -offset),
  vec2(offset, -offset)
);

const float kernel[9] = float[](
  0.0, -1.0, 0.0,
  -1.0, 4.0, -1.0,
  0.0, -1.0, 0.0
);

void main()
{
  vec3 color = vec3(0.0);
  for (int i = 0; i < 9; i++)
  {
    vec3 local = vec3(texture(screen, vs_texcoord.xy + offsets[i]));
    color += local * kernel[i];
  }
  FragColor = vec4(color, 1.0);
}
