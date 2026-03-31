#version 410

out vec4 frag_color;

uniform vec3 color;

void main()
{
    frag_color = vec4(color.rgb, 1.0);
}
