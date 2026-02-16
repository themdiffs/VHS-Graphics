#pragma once

#include "batteries/scene.h"
#include "batteries/lights.h"
#include "batteries/opengl.h"

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
    std::unique_ptr<ew::Model> suzanne;
    std::unique_ptr<ew::Shader> toon;
    std::unique_ptr<ew::Texture> texture;
    std::unique_ptr<ew::Texture> gradientTexture;
    
    std::unique_ptr<ew::Shader> postprocess;

    batteries::ambient_t ambient;
    batteries::light_t light;

    GLuint fbo;
    GLuint fbo_texture;
    GLuint fbo_depth;

    struct
    {
        glm::vec3 color1;
        glm::vec3 color2;

    } palette;
};