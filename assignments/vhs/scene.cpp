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

#include <algorithm>

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

struct
{
    float shininess = 128.0f;

    glm::vec3 lightDirection = {-0.5f, -1.0f, -0.5f};
    float min_bias = 0.005f;
    float max_bias = 0.05f;
    bool use_pcf = true;
    bool use_gouraud = true;
    bool show_cascades = false;
    bool use_cascades = true;

    // fog
    bool fog_enabled = true;
    glm::vec3 fog_color = {0.43f, 0.08f, 0.08f};
    float fog_near = 5.0f;
    float fog_far = 65.0f;
} debug;

Scene::Scene()
{
    suzanne = std::make_unique<ew::Model>("assets/models/suzanne.obj");
    toon = std::make_unique<ew::Shader>("assets/shaders/default_shadowmap_instance.vs", "assets/shaders/default_shadowmap_instance.fs");
    texture = std::make_unique<ew::Texture>("assets/ornament-color.jpg");
    brickTexture = std::make_unique<ew::Texture>("assets/brick_color.jpg");
    gradientTexture = std::make_unique<ew::Texture>("assets/textures/ZAtoon.png");

    depth = std::make_unique<ew::Shader>("assets/shaders/depth.vs", "assets/shaders/depth.fs");

    postprocess_none = std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/shaders/fullscreen.fs");
postprocess_crt = std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/shaders/crt.fs");

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

    cascade_splits[0] = 8.0f;
    cascade_splits[1] = 25.0f;
    cascade_splits[2] = 60.0f;
}

Scene::~Scene()
{
    glDeleteFramebuffers(1, &fbo);
    glDeleteFramebuffers(NUM_CASCADES, shadow_fbo);
}

void Scene::CreateDepthBuffer()
{
    for (int i = 0; i < NUM_CASCADES; i++)
    {
        glCreateFramebuffers(1, &shadow_fbo[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, shadow_fbo[i]);
        {
            glGenTextures(1, &shadow_depth[i]);
            glBindTexture(GL_TEXTURE_2D, shadow_depth[i]);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 800, 600, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_depth[i], 0);

            glDrawBuffers(0, nullptr);
            glReadBuffer(GL_NONE);
        }

        glBindTexture(GL_TEXTURE_2D, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            printf("Depthbuffer %d not complete!\n", i);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
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
    // this is fo the noise
    total_time += dt;

    glm::vec3 dir = glm::normalize(debug.lightDirection);
    light.position = -dir * 20.0f;
    lightMatrix[3] = glm::vec4(light.position, 1.0f);
}

auto matrix = glm::mat4(1.0f);

static glm::mat4 computeCascadeLightViewProj(
    float nearDist,
    float farDist,
    const glm::mat4& camProj,
    const glm::mat4& camView,
    const glm::vec3& lightDir)
{
    glm::mat4 subProj = camProj;
    subProj[2][2] = -(farDist + nearDist) / (farDist - nearDist);
    subProj[3][2] = -2.0f * farDist * nearDist / (farDist - nearDist);

    const glm::mat4 inv = glm::inverse(subProj * camView);

    glm::vec3 corners[8];
    int idx = 0;
    for (int x = 0; x < 2; x++)
        for (int y = 0; y < 2; y++)
            for (int z = 0; z < 2; z++)
            {
                glm::vec4 pt = inv * glm::vec4(2.0f*x-1.0f, 2.0f*y-1.0f, 2.0f*z-1.0f, 1.0f);
                corners[idx++] = glm::vec3(pt) / pt.w;
            }

    glm::vec3 center(0.0f);
    for (auto& c : corners) center += c;
    center /= 8.0f;

    glm::vec3 up = glm::abs(lightDir.y) < 0.99f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    const glm::mat4 lightView = glm::lookAt(center - lightDir * 20.0f, center, up);

    glm::vec4 first = lightView * glm::vec4(corners[0], 1.0f);
    float minX = first.x, maxX = first.x;
    float minY = first.y, maxY = first.y;
    float minZ = first.z, maxZ = first.z;

    for (auto& c : corners)
    {
        glm::vec4 lc = lightView * glm::vec4(c, 1.0f);
        minX = std::min(minX, lc.x); maxX = std::max(maxX, lc.x);
        minY = std::min(minY, lc.y); maxY = std::max(maxY, lc.y);
        minZ = std::min(minZ, lc.z); maxZ = std::max(maxZ, lc.z);
    }

    minZ -= 10.0f;
    maxZ += 10.0f;

    const glm::mat4 lightProj = glm::ortho(minX, maxX, minY, maxY, -maxZ, -minZ);
    return lightProj * lightView;
}

void Scene::Render(void)
{
    const auto view_proj = camera.Projection() * camera.View();

    // const auto light_proj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.01f, 100.0f);
    // const auto light_view = glm::lookAt(light.position, glm::vec3(0.0f), glm::vec3(0.0f, 10.0f, 0.0f));
    // const auto light_view_proj = light_proj * light_view;

    // shadow pass
    {
        const glm::vec3 lightDir = glm::normalize(debug.lightDirection);
        for (int i = 0; i < NUM_CASCADES; i++)
        {
            float nearDist = (i == 0) ? 0.01f : cascade_splits[i - 1];
            cascade_light_view_proj[i] = computeCascadeLightViewProj(
                nearDist, cascade_splits[i],
                camera.Projection(), camera.View(),
                lightDir);
        }

        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glEnable(GL_DEPTH_TEST);

        glViewport(0, 0, 800, 600);

        for (int i = 0; i < NUM_CASCADES; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, shadow_fbo[i]);
            glClear(GL_DEPTH_BUFFER_BIT);

            depth->use();
            depth->setMat4("model", matrix);
            depth->setMat4("light_view_proj", cascade_light_view_proj[i]);

            suzanne->draw();

            const auto plane_bottom = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
            depth->setMat4("model", plane_bottom);
            plane.draw();

            const auto plane_top = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 98.0f, 0.0f))
                * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            depth->setMat4("model", plane_top);
            plane.draw();

            const auto plane_front = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 48.0f, 50.0f))
                * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            depth->setMat4("model", plane_front);
            plane.draw();

            const auto plane_back = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 48.0f, -50.0f))
                * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            depth->setMat4("model", plane_back);
            plane.draw();

            const auto plane_right = glm::translate(glm::mat4(1.0f), glm::vec3(50.0f, 48.0f, 0.0f))
                * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            depth->setMat4("model", plane_right);
            plane.draw();

            const auto plane_left = glm::translate(glm::mat4(1.0f), glm::vec3(-50.0f, 48.0f, 0.0f))
                * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            depth->setMat4("model", plane_left);
            plane.draw();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

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
        glBindTexture(GL_TEXTURE_2D, brickTexture->getID());

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gradientTexture->getID());

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, shadow_depth[0]);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, shadow_depth[1]);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, shadow_depth[2]);

        toon->use();

        toon->setInt("texture0", 0);
        toon->setInt("gradientTex", 1);
        // toon->setInt("shadowMap", 2);
        toon->setInt("shadowMap[0]", 2);
        toon->setInt("shadowMap[1]", 3);
        toon->setInt("shadowMap[2]", 4);

        toon->setMat4("model", matrix);
        toon->setMat4("view_proj", view_proj);
        // toon->setMat4("light_view_proj", light_view_proj);
        toon->setVec3("camera_position", camera.position);
        toon->setVec2("screen_resolution", glm::vec2(800, 600) * (1.0f / float(vertexSettings.snap_resolution)));

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
        toon->setInt("use_gouraud", debug.use_gouraud);
        toon->setInt("show_cascades", debug.show_cascades);
        toon->setInt("use_cascades", debug.use_cascades);

        toon->setInt("fog.enabled", debug.fog_enabled);
        toon->setVec3("fog.color", debug.fog_color);
        toon->setFloat("fog.near", debug.fog_near);
        toon->setFloat("fog.far", debug.fog_far);

        toon->setVec3("light_position", light.position);
        toon->setVec3("light_color", light.color);
        toon->setFloat("shininess", debug.shininess);

        for (int i = 0; i < NUM_CASCADES; i++)
        {
            toon->setFloat("cascadeSplits[" + std::to_string(i) + "]", cascade_splits[i]);
            toon->setMat4("cascadeLightViewProj[" + std::to_string(i) + "]", cascade_light_view_proj[i]);
        }

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
        glDisable(GL_DEPTH_TEST);
        glClearColor(1.0f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // haha it says pp
        ew::Shader* pp;

        if (crt.enabled)
        {
            pp = postprocess_crt.get();
        }
        else
        {
            pp = postprocess_none.get();
        }

        // haha youre using pp
        pp->use();
        pp->setInt("screen", 0);

        if (crt.enabled)
        {
            pp->setFloat("u_curvature", crt.curvature);
            pp->setFloat("u_scanline_intensity", crt.scanline_intensity);
            pp->setFloat("u_scanline_count", crt.scanline_count);
            pp->setFloat("u_vignette_strength", crt.vignette_strength);
            pp->setFloat("u_brightness", crt.brightness);
            pp->setFloat("u_chromatic_aberration", crt.chromatic_aberration);
            pp->setFloat("u_noise_strength", crt.noise_strength);
            pp->setFloat("u_time", total_time);
        }

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

    if (ImGui::CollapsingHeader("Shading"))
    {
        ImGui::Checkbox("Gouraud", &debug.use_gouraud);
        ImGui::SliderFloat("Shininess", &debug.shininess, 1.0f, 256.0f);
    }

    if (ImGui::CollapsingHeader("Shadow Mapping"))
    {
        ImGui::SliderFloat("Min Bias", &debug.min_bias, 0.0f, 0.01f);
        ImGui::SliderFloat("Max Bias", &debug.max_bias, 0.0f, 0.1f);
        ImGui::Checkbox("PCF", &debug.use_pcf);
        ImGui::Checkbox("Use Cascades", &debug.use_cascades);
        ImGui::Checkbox("Show Cascades", &debug.show_cascades);
        ImGui::SliderFloat("Cascade 0 Split", &cascade_splits[0], 1.0f, 50.0f);
        ImGui::SliderFloat("Cascade 1 Split", &cascade_splits[1], 1.0f, 100.0f);
        ImGui::SliderFloat("Cascade 2 Split", &cascade_splits[2], 10.0f, 200.0f);
    }

    if (ImGui::CollapsingHeader("Fog"))
    {
        ImGui::Checkbox("Enabled", &debug.fog_enabled);
        ImGui::ColorEdit3("Fog Color", &debug.fog_color[0]);
        ImGui::SliderFloat("Fog Near", &debug.fog_near, 0.0f, 100.0f);
        ImGui::SliderFloat("Fog Far", &debug.fog_far, 0.0f, 200.0f);
    }

    if (ImGui::CollapsingHeader("CRT Shader"))
    {
        ImGui::Checkbox("Enabled", &crt.enabled);

        if (crt.enabled)
        {
            ImGui::SliderFloat("Curvature", &crt.curvature, 0.0f, 10.0f);
            ImGui::SliderFloat("Scanline Intensity", &crt.scanline_intensity, 0.0f, 1.0f);
            ImGui::SliderFloat("Scanline Count", &crt.scanline_count, 50.0f, 800.0f);
            ImGui::SliderFloat("Vignette", &crt.vignette_strength, 0.0f, 1.0f);
            ImGui::SliderFloat("Brightness", &crt.brightness, 0.5f, 2.0f);
            ImGui::SliderFloat("Chromatic Aberration", &crt.chromatic_aberration, 0.0f, 5.0f);
            ImGui::SliderFloat("Film Grain", &crt.noise_strength, 0.0f, 0.3f);
        }
    }

    if (ImGui::CollapsingHeader("Vertex Snapping"))
    {
        ImGui::Checkbox("Enabled", &vertexSettings.enabled);

        if(vertexSettings.enabled){
            ImGui::SliderInt("Vertex Resolution", &vertexSettings.snap_resolution, 1, 10);
        }
    }

    ImGui::End();
}