#version 410

out vec4 FragColor;

in vec2 vs_texcoord;

uniform sampler2D screen;
uniform float strength;

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
  -1.0, 5.0, -1.0,
  0.0, -1.0, 0.0
);

void main()
{
  vec3 sharpened = vec3(0.0);
  for (int i = 0; i < 9; i++)
  {
    vec3 local = vec3(texture(screen, vs_texcoord.xy + offsets[i]));
    sharpened += local * kernel[i];
  }

  vec3 original = vec3(texture(screen, vs_texcoord));
  vec3 color = mix(original, sharpened, strength);
  FragColor = vec4(color, 1.0);
}
