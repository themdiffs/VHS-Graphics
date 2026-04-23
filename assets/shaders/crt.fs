#version 330 core

in vec2 vs_texcoord;
out vec4 FragColor;

uniform sampler2D screen;

uniform float u_curvature;
uniform float u_scanline_intensity;
uniform float u_scanline_count;
uniform float u_vignette_strength;
uniform float u_brightness;
uniform float u_chromatic_aberration;
uniform float u_noise_strength;
uniform float u_time;

vec2 CurveUV(vec2 uv, float amount)
{
    // remap from 0-1 to -1 to 1 to make center the orign
    uv = uv * 2.0 - 1.0;
    // push the axes outward based on how far other acis is from center
    // .yx is like the swizzle node
    // corners get pushed in the most cause both axes are largest there
    vec2 offset = uv.yx * uv.yx * uv * amount * 0.1;
    uv += offset;
    return uv * 0.5 + 0.5;
}

void main()
{
    vec2 uv = vs_texcoord;

    // lens distortion
    vec2 curved_uv = CurveUV(uv, u_curvature);

    // turn area outside of distortion black
    if (curved_uv.x < 0.0 || curved_uv.x > 1.0 || curved_uv.y < 0.0 || curved_uv.y > 1.0)
    {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // chromatic aberration
    float ca = u_chromatic_aberration * 0.005;
    vec2 dir = curved_uv - 0.5;
    // sampling red a little further out in dir direction and blue a little further in
    float r = texture(screen, curved_uv + dir * ca).r;
    float g = texture(screen, curved_uv).g;
    float b = texture(screen, curved_uv - dir * ca).b;
    vec3 color = vec3(r, g, b);

    // scanlines
    // vertical sine wave makes bright and dark bands
    float scanline = sin(curved_uv.y * u_scanline_count * 3.14159) * 0.5 + 0.5;
    // tightens bright peaks to make lines rather than graidents
    scanline = pow(scanline, 0.8);
    scanline = mix(1.0, scanline, u_scanline_intensity);
    color *= scanline;

    // vignettee
    // vig uv is zero at edges and peaks in middle
    vec2 vig_uv = curved_uv * (1.0 - curved_uv.yx);
    //multiply aces to get bright in the middle and falling off toward edge
    float vignette = vig_uv.x * vig_uv.y * 15.0;
    vignette = pow(vignette, u_vignette_strength);
    color *= vignette;

    // film grain
    // noise offset per frame using screen pos
    vec2 noise_uv = curved_uv + fract(u_time * 0.07);
    // so apparently opengl does not have built in noise and you need wizardry to get it
    // i had to look this up and got it from https://thebookofshaders.com/10/
    float noise = fract(sin(dot(noise_uv, vec2(127.1, 311.7))) * 43758.5453);
    // centering on 0
    color += (noise - 0.5) * u_noise_strength;

    // brightness
    // yeah this ones pretty complicated i know
    color *= u_brightness;

    FragColor = vec4(color, 1.0);
}
