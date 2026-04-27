#version 330 core

in vec2 vs_texcoord;
out vec4 FragColor;

uniform sampler2D screen;
uniform sampler2D screenDepth;  // fbo_depth (GL_DEPTH24_STENCIL8)

uniform float u_camera_near;
uniform float u_camera_far;

uniform float u_curvature;
uniform float u_scanline_intensity;
uniform float u_scanline_count;
uniform float u_vignette_strength;
uniform float u_brightness;
uniform float u_chromatic_aberration;
uniform float u_noise_strength;
uniform float u_time;

uniform bool u_crt_enabled;
uniform bool u_pixelate_enabled;
uniform bool u_depth_based;
uniform float u_pixel_size;
uniform float u_depth_bias;
uniform float u_depth_power;

float linearizeDepth(float raw)
{
    // convert raw depth to ndc [-1,1]
    float z_ndc  = raw * 2.0 - 1.0;
    // getting view space depth
    float linear = (2.0 * u_camera_near * u_camera_far) / (u_camera_far + u_camera_near - z_ndc * (u_camera_far - u_camera_near));
    // remap to 0-1 range between near and far so 0 is right at the camera and 1 is at the far plane
    return clamp((linear - u_camera_near) / (u_camera_far - u_camera_near), 0.0, 1.0);
}

vec2 pixelate(vec2 uv)
{
    vec2 resolution = vec2(textureSize(screen, 0));

    if (u_depth_based)
    {
        // find which pixel block this uv falls into
        vec2 block_coord = floor(uv * resolution / u_pixel_size);
        vec2 block_origin = block_coord * u_pixel_size / resolution;

        // sample depth at the center and four corners of the block
        vec2 offsets[5];
        offsets[0] = vec2(0.5, 0.5);
        offsets[1] = vec2(0.1, 0.1);
        offsets[2] = vec2(0.9, 0.1);
        offsets[3] = vec2(0.1, 0.9);
        offsets[4] = vec2(0.9, 0.9);

        float min_raw_depth = 1.0;
        for (int i = 0; i < 5; i++)
        {
            vec2 sample_uv = block_origin + offsets[i] * u_pixel_size / resolution;
            sample_uv = clamp(sample_uv, vec2(0.0), vec2(1.0));
            min_raw_depth = min(min_raw_depth, texture(screenDepth, sample_uv).r);
        }

        float depth = linearizeDepth(min_raw_depth);

        // bias shifts where pixelation starts so nearby objects stay sharp
        float bias = clamp(u_depth_bias, 0.0, 0.999);
        float depth_adj = clamp((depth - bias) / (1.0 - bias), 0.0, 1.0);

        // power curve lets you control how pixel size grows with distance
        depth_adj = pow(depth_adj, u_depth_power);

        float pixel_size = max(1.0, u_pixel_size * depth_adj);

        // snap uv to the nearest pixel block of size
        return floor(uv * resolution / pixel_size) * pixel_size / resolution;
    }
    else
    {
        // flat pixelation
        return floor(uv * resolution / u_pixel_size) * u_pixel_size / resolution;
    }
}

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

    if (u_pixelate_enabled)
        uv = pixelate(uv);

    if (!u_crt_enabled)
    {
        FragColor = vec4(texture(screen, uv).rgb, 1.0);
        return;
    }

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
    float vignette = pow(vig_uv.x * vig_uv.y * 15.0, u_vignette_strength);
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