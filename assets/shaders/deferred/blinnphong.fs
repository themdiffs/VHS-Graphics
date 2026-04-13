#version 300 es

precision mediump float;
precision mediump int;

struct Light {
    vec3 color;
    vec3 position;
    float radius;
};

struct Material {
    float ambient;
    float diffuse;
    float specular;
    float shininess;
};

layout(location = 0) out vec4 frag_lighting;

uniform vec2 textureSize;
uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D g_albedo;
uniform Light light;
uniform Material material;
uniform vec3 camera_position;

float attenuate(float dist, float radius)
{
    float i = clamp(1.0 - pow(dist / radius, 4.0), 0.0, 1.0);
    return i * i;
}

vec3 blinnPhong(vec3 position, vec3 normal)
{
    vec3 view_dir = normalize(camera_position - position);
    vec3 light_dir = normalize(light.position - position);
    vec3 halfway_dir = normalize(light_dir + view_dir);

    float ndotl = max(dot(normal, light_dir), 0.0);
    float ndoth = max(dot(normal, halfway_dir), 0.0);

    vec3 diffuse = ndotl * vec3(material.diffuse);
    vec3 specular = pow(ndoth, material.shininess * 128.0) * vec3(material.specular);

    float atten = attenuate(length(light.position - position), light.radius);
    return (diffuse + specular) * atten * light.color;
}

void main()
{
    vec2 uv = gl_FragCoord.xy / textureSize;

    vec3 position = texture(g_position, uv).xyz;
    vec3 normal = texture(g_normal, uv).xyz;

    vec3 lighting = blinnPhong(position, normal);
    frag_lighting = vec4(lighting, 1.0);
}
