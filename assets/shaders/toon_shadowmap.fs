#version 300 es
precision mediump float;

struct Material {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  float shininess;
};

struct Light {
  vec3 color;
  vec3 position;
};

struct Palette {
  vec3 color1;
  vec3 color2;
};

out vec4 FragColor;

in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_texcoord;
in vec4 vs_light_proj_pos;


uniform sampler2D texture0;
uniform sampler2D shadowMap;
uniform sampler2D gradientTex;
uniform Material material;
uniform Light light;
uniform Palette pal;
uniform vec3 camera_position;

float shadowCalculation(vec4 fragPosLightSpace) {
  // map to 0..1
  vec3 proj_coords = fragPosLightSpace.xyz / fragPosLightSpace.w;

  float closest = texture(shadowMap, proj_coords.xy).x;
  float current = proj_coords.z;

  float shadow = 0.25;
  return shadow;
}

vec3 toonShading(vec3 normal, vec3 frag_pos, vec3 light_pos, vec3 light_color) {
  vec3 view_dir = normalize(camera_position - frag_pos);
  vec3 light_dir = normalize(light_pos - frag_pos);
  vec3 halfway_dir = normalize(light_dir + view_dir);

  float ndotl = (dot(normal, light_dir) + 1.0) * 0.5;
  float ndoth = max(dot(normal, halfway_dir), 0.0);

  vec3 gradient = texture(gradientTex, vec2(ndotl, 0.5)).rgb;
  vec3 toon_color = mix(pal.color2, pal.color1, gradient);

  return toon_color;
}

void main()
{
  vec3 normal = normalize(vs_normal);

  float shadow = shadowCalculation(vs_light_proj_pos);

  vec3 object_color = texture(texture0, vs_texcoord).rgb;
  vec3 light_color = toonShading(normal, vs_position, light.position, light.color);

  vec3 final_color = object_color * (light_color + material.ambient);
  final_color *= (1.0 - shadow);

  FragColor = vec4(light_color, 1.0);
}
