#include "scene.h"

#include "imgui/imgui.h"
#include "imguizmo/imguizmo.h"

#include "gl3w/include/GL/gl3w.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "batteries/materials.h"
#include "batteries/math.h"
#include "batteries/opengl.h"
#include "ew/procGen.h"

struct FullScreenQuad
{
    GLuint vao;
    GLuint vbo;

    void Initialize()
    {
        float vertices[] = {
            // pos (x, y),
            // texcoord (u, v)
            // triangle 1
            -1.0f,
            1.0f,
            0.0f,
            1.0f,
            -1.0f,
            -1.0f,
            0.0f,
            0.0f,
            1.0f,
            -1.0f,
            1.0f,
            0.0f,

            // triangle 2
            -1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            -1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

        // pos (x, y)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        // texcoord (u, v)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(sizeof(float) * 2));

        // always last.
        glBindVertexArray(0);
    }
} fullscreen_quad;

glm::mat4 lightMatrix = glm::mat4(1.0f);
const glm::vec4 backgroundColor = glm::vec4(0.6f, 0.8f, 0.92f, 1.0f);

static int current_effect = 0;
static const char* effect_names[] = {
    "None",
    "Greyscale",
    "Blur",
    "Invert",
    "Edge Detect",
    "Sharpen",
    "Chromatic Aberration",
    "Vignette",
    "Lens Distortion",
    "Film Grain",
    "Gamma Correction",
};

struct
{
    float shininess = 128.0f;
    float blur_strength = 16.0f;
    float sharpen_strength = 1.0f;
    float chromatic_offset = 0.005f;
    float vignette_intensity = 0.5f;
    float lens_strength = 0.5f;
    float grain_strength = 0.15f;
    float gamma = 2.2f;

    glm::vec3 lightDirection = {-0.5f, -1.0f, -0.5f};
    float min_bias = 0.005f;
    float max_bias = 0.05f;
    bool use_pcf = true;

    // fog
    bool fog_enabled = true;
    glm::vec3 fog_color = {0.6f, 0.8f, 0.92f};
    float fog_near = 10.0f;
    float fog_far = 80.0f;
} debug;

Scene::Scene()
{
    suzanne = std::make_unique<ew::Model>("assets/models/suzanne.obj");
    toon = std::make_unique<ew::Shader>("assets/shaders/default_shadowmap_instance.vs", "assets/shaders/default_shadowmap_instance.fs");
    texture = std::make_unique<ew::Texture>("assets/ornament-color.jpg");
    gradientTexture = std::make_unique<ew::Texture>("assets/textures/ZAtoon.png");

    depth = std::make_unique<ew::Shader>("assets/shaders/depth.vs", "assets/shaders/depth.fs");

    postprocess_none = std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/shaders/fullscreen.fs");
    postprocess_greyscale = std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/shaders/greyscale.fs");
    postprocess_blur = std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/shaders/blur.fs");
    postprocess_invert = std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/shaders/invert.fs");
    postprocess_edgedetect = std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/shaders/edgedetect.fs");
    postprocess_sharpen = std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/shaders/sharpen.fs");
    postprocess_chromatic = std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/shaders/chromatic.fs");
    postprocess_vignette = std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/shaders/vignette.fs");
    postprocess_lensdistortion = std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/shaders/lensdistortion.fs");
    postprocess_filmgrain = std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/shaders/filmgrain.fs");
    postprocess_gammacorrection = std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/shaders/gammacorrection.fs");

    light = {
        .brightness = 1.0f,
        .color = {1.0f, 1.0f, 1.0f},
        .position = {0.0f, 2.0f, 0.0f},
    };

    palette = {
        .color1 = {1.0f, 0.0f, 1.0f},
        .color2 = {0.0f, 0.0f, 1.0f},
    };

    fullscreen_quad.Initialize();

    CreateFrameBuffer();
    CreateDepthBuffer();

    plane.load(ew::createPlane(100, 100, 1));
}

Scene::~Scene()
{
    glDeleteFramebuffers(1, &fbo);
    glDeleteFramebuffers(1, &shadow_fbo);
}

void Scene::CreateDepthBuffer()
{
    glCreateFramebuffers(1, &shadow_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, shadow_fbo);
    {
        glGenTextures(1, &shadow_depth);
        glBindTexture(GL_TEXTURE_2D, shadow_depth);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 800, 600, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_depth, 0);

        glDrawBuffers(0, nullptr);
        glReadBuffer(GL_NONE);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("Depthbuffer not complete!\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Scene::CreateFrameBuffer()
{
    glCreateFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    {
        glGenTextures(1, &fbo_texture);
        glBindTexture(GL_TEXTURE_2D, fbo_texture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture, 0);

        glGenTextures(1, &fbo_depth);
        glBindTexture(GL_TEXTURE_2D, fbo_depth);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 800, 600, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fbo_depth, 0);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("Framebuffer not complete!\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Scene::Update(float dt)
{
    batteries::Scene::Update(dt);

    glm::vec3 dir = glm::normalize(debug.lightDirection);
    light.position = -dir * 20.0f;
    lightMatrix[3] = glm::vec4(light.position, 1.0f);
}

auto matrix = glm::mat4(1.0f);

void Scene::Render(void)
{
    const auto view_proj = camera.Projection() * camera.View();

    const auto light_proj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.01f, 100.0f);
    const auto light_view = glm::lookAt(light.position, glm::vec3(0.0f), glm::vec3(0.0f, 10.0f, 0.0f));
    const auto light_view_proj = light_proj * light_view;

    // shadow pass
    glBindFramebuffer(GL_FRAMEBUFFER, shadow_fbo);
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glEnable(GL_DEPTH_TEST);

        glViewport(0, 0, 800, 600);

        glClear(GL_DEPTH_BUFFER_BIT);

        depth->use();
        depth->setMat4("model", matrix);
        depth->setMat4("light_view_proj", light_view_proj);

        suzanne->draw();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // render scene to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    {
        glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST);

        glViewport(0, 0, 800, 600);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->getID());

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gradientTexture->getID());

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, shadow_depth);

        toon->use();

        toon->setInt("texture0", 0);
        toon->setInt("gradientTex", 1);
        toon->setInt("shadowMap", 2);

        toon->setMat4("model", matrix);
        toon->setMat4("view_proj", view_proj);
        toon->setMat4("light_view_proj", light_view_proj);
        toon->setVec3("camera_position", camera.position);

        toon->setVec3("light.position", light.position);
        toon->setVec3("light.color", light.color);
        toon->setFloat("material.shininess", debug.shininess);

        toon->setVec3("pal.color1", palette.color1);
        toon->setVec3("pal.color2", palette.color2);

        toon->setVec3("material.diffuse", glm::vec3(1));
        toon->setVec3("material.specular", glm::vec3(1));

        toon->setFloat("min_bias", debug.min_bias);
        toon->setFloat("max_bias", debug.max_bias);
        toon->setInt("use_pcf", debug.use_pcf);

        toon->setInt("fog.enabled", debug.fog_enabled);
        toon->setVec3("fog.color", debug.fog_color);
        toon->setFloat("fog.near", debug.fog_near);
        toon->setFloat("fog.far", debug.fog_far);

        suzanne->draw();

        // bottom
        const auto plane_bottom = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
        toon->setMat4("model", plane_bottom);
        plane.draw();

        // top
        const auto plane_top = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 98.0f, 0.0f))
            * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        toon->setMat4("model", plane_top);
        plane.draw();

        // front
        const auto plane_front = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 48.0f, 50.0f))
            * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        toon->setMat4("model", plane_front);
        plane.draw();

        // back
        const auto plane_back = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 48.0f, -50.0f))
            * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        toon->setMat4("model", plane_back);
        plane.draw();

        // right
        const auto plane_right = glm::translate(glm::mat4(1.0f), glm::vec3(50.0f, 48.0f, 0.0f))
            * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        toon->setMat4("model", plane_right);
        plane.draw();

        // left
        const auto plane_left = glm::translate(glm::mat4(1.0f), glm::vec3(-50.0f, 48.0f, 0.0f))
            * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        toon->setMat4("model", plane_left);
        plane.draw();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // post processing pipeline
    {
        switch (current_effect)
        {
        case 0:
            postprocess_none->use();
            postprocess_none->setInt("screen", 0);
            break;
        case 1:
            postprocess_greyscale->use();
            postprocess_greyscale->setInt("screen", 0);
            break;
        case 2:
            postprocess_blur->use();
            postprocess_blur->setInt("screen", 0);
            postprocess_blur->setFloat("strength", debug.blur_strength);
            break;
        case 3:
            postprocess_invert->use();
            postprocess_invert->setInt("screen", 0);
            break;
        case 4:
            postprocess_edgedetect->use();
            postprocess_edgedetect->setInt("screen", 0);
            break;
        case 5:
            postprocess_sharpen->use();
            postprocess_sharpen->setInt("screen", 0);
            postprocess_sharpen->setFloat("strength", debug.sharpen_strength);
            break;
        case 6:
            postprocess_chromatic->use();
            postprocess_chromatic->setInt("screen", 0);
            postprocess_chromatic->setFloat("offset", debug.chromatic_offset);
            break;
        case 7:
            postprocess_vignette->use();
            postprocess_vignette->setInt("screen", 0);
            postprocess_vignette->setFloat("intensity", debug.vignette_intensity);
            break;
        case 8:
            postprocess_lensdistortion->use();
            postprocess_lensdistortion->setInt("screen", 0);
            postprocess_lensdistortion->setFloat("strength", debug.lens_strength);
            break;
        case 9:
            postprocess_filmgrain->use();
            postprocess_filmgrain->setInt("screen", 0);
            postprocess_filmgrain->setFloat("time", (float)time.absolute);
            postprocess_filmgrain->setFloat("strength", debug.grain_strength);
            break;
        case 10:
            postprocess_gammacorrection->use();
            postprocess_gammacorrection->setInt("screen", 0);
            postprocess_gammacorrection->setFloat("gamma", debug.gamma);
            break;
        }

        glDisable(GL_DEPTH_TEST);
        glClearColor(1.0f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(fullscreen_quad.vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fbo_texture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}

void Scene::Debug(void)
{
    ImGuizmo::BeginFrame();
    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
    ImGuizmo::SetRect(0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);

    auto* view = glm::value_ptr(camera.View());
    auto* proj = glm::value_ptr(camera.Projection());

    ImGuizmo::Manipulate(
        view,
        proj,
        ImGuizmo::TRANSLATE,
        ImGuizmo::WORLD,
        glm::value_ptr(lightMatrix));

    if (ImGuizmo::IsUsing())
    {
        light.position = glm::vec3(lightMatrix[3]);
        debug.lightDirection = -glm::normalize(light.position);
    }

    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::CollapsingHeader("Light Direction"))
    {
        ImGui::SliderFloat("Dir X", &debug.lightDirection.x, -1.0f, 1.0f);
        ImGui::SliderFloat("Dir Y", &debug.lightDirection.y, -1.0f, 1.0f);
        ImGui::SliderFloat("Dir Z", &debug.lightDirection.z, -1.0f, 1.0f);
        ImGui::ColorEdit3("Light Color", &light.color.x);
    }

    if (ImGui::CollapsingHeader("Shadow Mapping"))
    {
        ImGui::SliderFloat("Min Bias", &debug.min_bias, 0.0f, 0.01f);
        ImGui::SliderFloat("Max Bias", &debug.max_bias, 0.0f, 0.1f);
        ImGui::Checkbox("PCF", &debug.use_pcf);
    }

    if (ImGui::CollapsingHeader("Toon Shading"))
    {
        ImGui::SliderFloat("Shininess", &debug.shininess, 2.0f, 1024.0f);
        ImGui::ColorEdit3("Color1", &palette.color1[0]);
        ImGui::ColorEdit3("Color2", &palette.color2[0]);
    }

    if (ImGui::CollapsingHeader("Fog"))
    {
        ImGui::Checkbox("Enabled", &debug.fog_enabled);
        ImGui::ColorEdit3("Fog Color", &debug.fog_color[0]);
        ImGui::SliderFloat("Fog Near", &debug.fog_near, 0.0f, 100.0f);
        ImGui::SliderFloat("Fog Far", &debug.fog_far, 0.0f, 200.0f);
    }

    if (ImGui::CollapsingHeader("Post Processing"))
    {
        ImGui::Combo("Effect", &current_effect, effect_names, IM_ARRAYSIZE(effect_names));

        switch (current_effect)
        {
        case 2:
            ImGui::SliderFloat("Blur Strength", &debug.blur_strength, 0.0f, 10.0f);
            break;
        case 5:
            ImGui::SliderFloat("Sharpen Strength", &debug.sharpen_strength, 0.1f, 5.0f);
            break;
        case 6:
            ImGui::SliderFloat("Aberration Offset", &debug.chromatic_offset, -0.02f, 0.02f);
            break;
        case 7:
            ImGui::SliderFloat("Vignette Intensity", &debug.vignette_intensity, 0.0f, 1.5f);
            break;
        case 8:
            ImGui::SliderFloat("Distortion", &debug.lens_strength, -5.0f, 5.0f);
            break;
        case 9:
            ImGui::SliderFloat("Grain Strength", &debug.grain_strength, 0.0f, 0.5f);
            break;
        case 10:
            ImGui::SliderFloat("Gamma", &debug.gamma, 0.5f, 4.0f);
            break;
        }
    }

    ImGui::End();
}