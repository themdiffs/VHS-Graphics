#pragma once

#include "batteries/lights.h"
#include "batteries/opengl.h"
#include "batteries/scene.h"

#include "ew/model.h"
#include "ew/shader.h"
#include "ew/texture.h"

class Scene final : public batteries::Scene
{
  public:
    Scene();
    virtual ~Scene();

    void Update(float dt);
    void Render(void);
    void Debug(void);

  private:
    void CreateFrameBuffer();
    void CreateDepthBuffer();

    std::unique_ptr<ew::Model> suzanne;
    std::unique_ptr<ew::Shader> toon;
    std::unique_ptr<ew::Texture> texture;
    std::unique_ptr<ew::Texture> brickTexture;
    std::unique_ptr<ew::Texture> gradientTexture;

    std::unique_ptr<ew::Shader> depth;

    std::unique_ptr<ew::Shader> postprocess_none;
    std::unique_ptr<ew::Shader> postprocess_crt;

    batteries::ambient_t ambient;
    batteries::light_t light;

    GLuint fbo;
    GLuint fbo_texture;
    GLuint fbo_depth;

    // depth buffer
    // GLuint shadow_fbo;
    // GLuint shadow_texture;
    // GLuint shadow_depth;
    static const int NUM_CASCADES = 3;
    GLuint shadow_fbo[NUM_CASCADES];
    GLuint shadow_depth[NUM_CASCADES];
    glm::mat4 cascade_light_view_proj[NUM_CASCADES];
    float cascade_splits[NUM_CASCADES];

    ew::Mesh plane;

    struct
    {
        glm::vec3 color1;
        glm::vec3 color2;
    } palette;

    // crt setting s stuff
    struct CRTSettings
    {
        bool enabled = true;
        float curvature = 2.0f;
        float scanline_intensity = 0.1f;
        float scanline_count = 300.0f;
        float vignette_strength = 0.3f;
        float brightness = 1.0f;
        float chromatic_aberration = 1.5f;
        float noise_strength = 0.08f;
    } crt;

    struct VertexSnapSettings{
      bool enabled = true;
      int snap_resolution = 1;
    } vertexSettings;

    float total_time = 0.0f;
};