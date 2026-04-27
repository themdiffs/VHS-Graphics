#version 410
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

struct Fog {
  vec3 color;
  
  float near;
  float far;
  float density;
  
  bool enabled;
};

out vec4 FragColor;

in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_texcoord;
in vec4 vs_light_proj_pos;

// Gouraud 
in vec3 vs_color;
uniform sampler2D texture0;
// uniform sampler2D shadowMap;
uniform sampler2D shadowMap[3];
uniform sampler2D gradientTex;
uniform Material material;
uniform Light light;
uniform Palette pal;
uniform Fog fog;
uniform vec3 camera_position;
uniform float min_bias;
uniform float max_bias;
uniform bool use_pcf;
uniform bool use_gouraud;
uniform bool show_cascades;
uniform bool use_cascades;
uniform float cascadeSplits[3];
uniform mat4 cascadeLightViewProj[3];
noperspective in vec2 vs_texcoord_affine;
uniform bool use_affine;

float shadowCalculation(vec4 fragPosLightSpace, sampler2D map)
{
  vec3 proj_coords = fragPosLightSpace.xyz / fragPosLightSpace.w;
  proj_coords = proj_coords * 0.5 + 0.5;

  if (proj_coords.z > 1.0)
    return 0.0;

  float current_depth = proj_coords.z;

  vec3 normal = normalize(vs_normal);
  vec3 light_dir = normalize(light.position - vs_position);
  float bias = max(max_bias * (1.0 - dot(normal, light_dir)), min_bias);

  float shadow = 0.0;

  if (use_pcf)
  {
    vec2 texel_size = 1.0 / vec2(textureSize(map, 0));
    for (int x = -1; x <= 1; ++x)
    {
      for (int y = -1; y <= 1; ++y)
      {
        float pcf_depth = texture(map, proj_coords.xy + vec2(x, y) * texel_size).r;
        shadow += (current_depth - bias) > pcf_depth ? 1.0 : 0.0;
      }
    }
    shadow /= 9.0;
  }
  else
  {
    float closest_depth = texture(map, proj_coords.xy).r;
    shadow = (current_depth - bias) > closest_depth ? 1.0 : 0.0;
  }

  return shadow;
}

void main()
{
  vec2 uv = use_affine ? vs_texcoord_affine : vs_texcoord;
//   vec3 normal = normalize(vs_normal);
  float dist = length(camera_position - vs_position);

  int cascade = 0;
  if      (dist > cascadeSplits[1]) cascade = 2;
  else if (dist > cascadeSplits[0]) cascade = 1;
  if (!use_cascades) cascade = 2;

  float shadow;
  if (cascade == 0)
    shadow = shadowCalculation(cascadeLightViewProj[0] * vec4(vs_position, 1.0), shadowMap[0]);
  else if (cascade == 1)
    shadow = shadowCalculation(cascadeLightViewProj[1] * vec4(vs_position, 1.0), shadowMap[1]);
  else
    shadow = shadowCalculation(cascadeLightViewProj[2] * vec4(vs_position, 1.0), shadowMap[2]);

//   vec3 light_dir = normalize(light.position - vs_position);
//   vec3 view_dir = normalize(camera_position - vs_position);
//   vec3 halfway_dir = normalize(light_dir + view_dir);
  vec3 texColor = texture(texture0, uv).rgb;
  // vec3 ambient = 0.1 * texColor;
  // float diff = max(dot(normal, light_dir), 0.0);
  // vec3 diffuse = diff * light.color * texColor;
  // float spec = pow(max(dot(normal, halfway_dir), 0.0), material.shininess);
  // vec3 specular = spec * light.color * material.specular;
  // vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);
  // vec3 lighting = vs_color * (1.0 - shadow);
  vec3 ambient = 0.1 * vec3(1.0);

  vec3 shade_color;
  if (use_gouraud)
  {
    shade_color = vs_color;
  }
  else
  {
    vec3 n = normalize(vs_normal);
    vec3 l = normalize(light.position - vs_position);
    vec3 v = normalize(camera_position - vs_position);
    vec3 h = normalize(l + v);
    float diff = max(dot(n, l), 0.0);
    float spec = pow(max(dot(n, h), 0.0), material.shininess);
    shade_color = ambient + diff * light.color + (spec * 0.3) * light.color;
  }

  vec3 lighting = ambient + (shade_color - ambient) * (1.0 - shadow);
  if (fog.enabled)
  {
    float fog_factor = clamp((fog.far - dist) / (fog.far - fog.near), 0.0, 1.0);
    lighting = mix(fog.color, lighting, fog_factor);
  }
  // FragColor = vec4(lighting, 1.0);
  FragColor = vec4(lighting * texColor, 1.0);
  if (show_cascades)
  {
    if      (cascade == 0) FragColor = mix(FragColor, vec4(1.0, 0.2, 0.2, 1.0), 0.3);
    else if (cascade == 1) FragColor = mix(FragColor, vec4(0.2, 1.0, 0.2, 1.0), 0.3);
    else                   FragColor = mix(FragColor, vec4(0.2, 0.2, 1.0, 1.0), 0.3);
  }
}