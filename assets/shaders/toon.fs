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

out vec4 FragColor;

in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_texcoord;
in mat3 vs_tbn;

uniform sampler2D texture0;
uniform sampler2D normal_map;
uniform Material material;
uniform Light light;
uniform vec3 camera_position;

vec3 blinnPhong(vec3 normal, vec3 frag_pos, vec3 light_pos, vec3 light_color) {
  vec3 view_dir = normalize(camera_position - frag_pos);
  vec3 light_dir = normalize(light_pos - frag_pos);
  vec3 halfway_dir = normalize(light_dir + view_dir);

  float NdotL = max(dot(normal, light_dir), 0.0);
  float NdotH = max(dot(normal, halfway_dir), 0.0);

  vec3 diffuse = NdotL * material.diffuse;
  vec3 specular = pow(NdotH, material.shininess) * material.specular;

  return (diffuse + specular) * light_color;
}

void main()
{
  vec3 normal = texture(normal_map, vs_texcoord).rgb * 2.0 - 1.0;
  normal = normalize(vs_tbn * normal);

  vec3 object_color = texture(texture0, vs_texcoord).rgb;
  vec3 light_color = blinnPhong(normal, vs_position, light.position, light.color);
  vec3 final_color = object_color * (light_color + material.ambient);

  FragColor = vec4(final_color, 1.0);
}
