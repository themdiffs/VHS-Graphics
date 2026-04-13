#include "scene.h"

#include "batteries/math.h"
#include "batteries/opengl.h"

#include "ew/procGen.h"

#include "imgui/imgui.h"

#include "sokol/sokol_app.h"

constexpr int kFramebufferWidth = 800;
constexpr int kFramebufferHeight = 600;

constexpr float orbit_radius = 2.0f;

struct FullscreenQuad
{
    GLuint vao;
    GLuint vbo;

    void Initialize()
    {
        // clang-format off
        float quad_vertices[] = {
            // pos (x, y) texcoord (u, v)
            -1.0f,  1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 1.0f, 0.0f,

            -1.0f,  1.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 1.0f,
        };
        // clang-format on

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(sizeof(float) * 2));

        glBindVertexArray(0);
    }
} fullscreen_quad;

struct GBuffer
{
    GLuint fbo;
    GLuint position;
    GLuint normal;
    GLuint albedo;
    GLuint depth;

    void Initialize()
    {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // position attachment (world-space, needs float precision)
        glGenTextures(1, &position);
        glBindTexture(GL_TEXTURE_2D, position);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, kFramebufferWidth, kFramebufferHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, position, 0);

        // normal attachment (world-space, needs float precision)
        glGenTextures(1, &normal);
        glBindTexture(GL_TEXTURE_2D, normal);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, kFramebufferWidth, kFramebufferHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normal, 0);

        // albedo attachment (0-1 color, 8-bit is fine)
        glGenTextures(1, &albedo);
        glBindTexture(GL_TEXTURE_2D, albedo);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kFramebufferWidth, kFramebufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, albedo, 0);

        GLenum attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
        glDrawBuffers(3, attachments);

        // depth attachment
        glGenTextures(1, &depth);
        glBindTexture(GL_TEXTURE_2D, depth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, kFramebufferWidth, kFramebufferHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depth, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            printf("GBuffer framebuffer incomplete\n");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
} gbuffer;

struct LightVolumeBuffer
{
    GLuint fbo;
    GLuint color;
    GLuint depth;

    void Initialize()
    {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        glGenTextures(1, &color);
        glBindTexture(GL_TEXTURE_2D, color);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, kFramebufferWidth, kFramebufferHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);

        glGenTextures(1, &depth);
        glBindTexture(GL_TEXTURE_2D, depth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, kFramebufferWidth, kFramebufferHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depth, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            printf("LightVolume framebuffer incomplete\n");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
} lightvolumeBuffer;

struct Material
{
    float ambient = 1.0f;
    float diffuse = 0.5f;
    float specular = 0.5f;
    float shininess = 0.5f;
} material;

struct
{
    int width = 1;
    float light_radius = 2.5f;
    bool draw_light_volume = false;
} debug;

Scene::Scene()
{
    suzanne = std::make_unique<ew::Model>("assets/models/suzanne.obj");
    geometry = std::make_unique<ew::Shader>("assets/shaders/deferred/geometry.vs", "assets/shaders/deferred/geometry.fs");
    blinnphong = std::make_unique<ew::Shader>("assets/shaders/deferred/blinnphong.vs", "assets/shaders/deferred/blinnphong.fs");
    noprocess = std::make_unique<ew::Shader>("assets/shaders/deferred/default.vs", "assets/shaders/deferred/default.fs");
    lightsphere = std::make_unique<ew::Shader>("assets/shaders/deferred/light.vs", "assets/shaders/deferred/light.fs");

    texture = std::make_unique<ew::Texture>("assets/ornament-color.jpg");

    sphere.load(ew::createSphere(1.0f, 8));

    ambient = {
        .intensity = 1.0f,
        .color = {0.5f, 0.5f, 0.5f},
    };

    gbuffer.Initialize();
    lightvolumeBuffer.Initialize();
    fullscreen_quad.Initialize();

    InitializeInstanceData();
}

Scene::~Scene()
{
}

void Scene::InitializeInstanceData(void)
{
    auto width = debug.width;
    auto size = (width - (-width) + 1) * (width - (-width) + 1);
    model_instances.resize(size);
    light_instances.resize(size);

    auto i = 0;
    for (auto x = -debug.width; x <= debug.width; x++)
    {
        for (auto y = -debug.width; y <= debug.width; y++, i++)
        {
            const auto position = glm::vec3(x * 3.0f, 0, y * 3.0f);
            const auto orbit = batteries::random_point_on_sphere();

            light_instances[i] = {
                .color = batteries::random_color(),
                .position = glm::vec4(position, 1.0f) + orbit * orbit_radius,
            };

            model_instances[i] = batteries::random_model_matrix(position);
        }
    }
}

void Scene::Update(float dt)
{
    batteries::Scene::Update(dt);
}

void Scene::Render(void)
{
    const auto view_proj = camera.Projection() * camera.View();

    // geometry pass: render scene into g-buffer
    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.fbo);
    {
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glViewport(0, 0, kFramebufferWidth, kFramebufferHeight);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        geometry->use();
        geometry->setMat4("view_proj", view_proj);

        auto i = 0;
        for (auto x = -debug.width; x <= debug.width; x++)
        {
            for (auto y = -debug.width; y <= debug.width; y++, i++)
            {
                geometry->setMat4("model", model_instances[i]);
                suzanne->draw();
            }
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // lighting pass: render light volumes with additive blending
    glBindFramebuffer(GL_FRAMEBUFFER, lightvolumeBuffer.fbo);
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_FUNC_ADD);

        glDisable(GL_DEPTH_TEST);
        glCullFace(GL_FRONT);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbuffer.position);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gbuffer.normal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gbuffer.albedo);

        blinnphong->use();
        blinnphong->setMat4("view_proj", view_proj);
        blinnphong->setVec3("camera_position", camera.position);
        blinnphong->setVec2("textureSize", glm::vec2(kFramebufferWidth, kFramebufferHeight));

        blinnphong->setInt("g_position", 0);
        blinnphong->setInt("g_normal", 1);
        blinnphong->setInt("g_albedo", 2);

        blinnphong->setFloat("material.ambient", material.ambient);
        blinnphong->setFloat("material.diffuse", material.diffuse);
        blinnphong->setFloat("material.specular", material.specular);
        blinnphong->setFloat("material.shininess", material.shininess);

        for (size_t i = 0; i < light_instances.size(); i++)
        {
            const auto scale = debug.light_radius;
            blinnphong->setMat4("model", glm::translate(glm::mat4(1.0f), light_instances[i].position) * glm::scale(glm::mat4(1.0f), glm::vec3(scale)));
            blinnphong->setVec3("light.position", light_instances[i].position);
            blinnphong->setVec3("light.color", light_instances[i].color);
            blinnphong->setFloat("light.radius", debug.light_radius);

            sphere.draw();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // composite pass: fullscreen quad combining albedo and lighting
    {
        noprocess->use();
        noprocess->setInt("g_albedo", 0);
        noprocess->setInt("g_lighting", 1);

        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glCullFace(GL_BACK);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glViewport(0, 0, sapp_widthf(), sapp_heightf());

        glBindVertexArray(fullscreen_quad.vao);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbuffer.albedo);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, lightvolumeBuffer.color);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindVertexArray(0);
    }

    // light orb pass: forward-render small spheres at each light position
    {
        glEnable(GL_DEPTH_TEST);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer.fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, kFramebufferWidth, kFramebufferHeight, 0, 0, sapp_widthf(), sapp_heightf(), GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        lightsphere->use();

        for (size_t i = 0; i < light_instances.size(); i++)
        {
            lightsphere->setMat4("view_proj", view_proj);
            lightsphere->setMat4("model", glm::translate(glm::mat4(1.0f), light_instances[i].position) * glm::scale(glm::mat4(1.0f), glm::vec3(0.25f)));
            lightsphere->setVec3("color", light_instances[i].color);
            sphere.draw();

            if (debug.draw_light_volume)
            {
                const auto scale = debug.light_radius;
                lightsphere->setMat4("model", glm::translate(glm::mat4(1.0f), light_instances[i].position) * glm::scale(glm::mat4(1.0f), glm::vec3(scale)));
                sphere.draw(ew::DrawMode::LINES);
            }
        }
    }
}

void Scene::Debug(void)
{
    cameracontroller.Debug();

    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Checkbox("Paused", &time.paused);
    ImGui::SliderFloat("Time Factor", &time.factor, 0.0f, 10.0f);

    if (ImGui::SliderInt("Width", &debug.width, 1, 100))
    {
        InitializeInstanceData();
    }

    if (ImGui::CollapsingHeader("Lights"))
    {
        ImGui::Checkbox("Draw Volumes", &debug.draw_light_volume);
        ImGui::SliderFloat("Light Radius", &debug.light_radius, 1.0f, 100.0f);
    }

    if (ImGui::CollapsingHeader("Material"))
    {
        ImGui::SliderFloat("Ambient", &material.ambient, 0.0f, 1.0f);
        ImGui::SliderFloat("Diffuse", &material.diffuse, 0.0f, 1.0f);
        ImGui::SliderFloat("Specular", &material.specular, 0.0f, 1.0f);
        ImGui::SliderFloat("Shininess", &material.shininess, 0.0f, 1.0f);
    }

    if (ImGui::CollapsingHeader("G-Buffer"))
    {
        ImVec2 uv_min(0.0f, 1.0f);
        ImVec2 uv_max(1.0f, 0.0f);

        ImGui::Text("Lighting:");
        ImGui::Image((ImTextureID)(intptr_t)lightvolumeBuffer.color, ImVec2(200, 150), uv_min, uv_max);

        ImGui::Text("Albedo:");
        ImGui::Image((ImTextureID)(intptr_t)gbuffer.albedo, ImVec2(200, 150), uv_min, uv_max);

        ImGui::Text("Position:");
        ImGui::Image((ImTextureID)(intptr_t)gbuffer.position, ImVec2(200, 150), uv_min, uv_max);

        ImGui::Text("Normal:");
        ImGui::Image((ImTextureID)(intptr_t)gbuffer.normal, ImVec2(200, 150), uv_min, uv_max);

        ImGui::Text("Depth:");
        ImGui::Image((ImTextureID)(intptr_t)gbuffer.depth, ImVec2(200, 150), uv_min, uv_max);
    }

    ImGui::End();
}
