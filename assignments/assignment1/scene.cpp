#include "scene.h"

#include "imgui/imgui.h"
#include "imguizmo/imguizmo.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "batteries/materials.h"
#include "batteries/math.h"
#include "batteries/opengl.h"

glm::mat4 lightMatrix = glm::mat4(1.0f);

const glm::vec4 backgroundColor = glm::vec4(0.6f, 0.8f, 0.92f, 1.0f);

struct {
    float shininess = 128.0f;
    float kd = 0.5f;
    float ks = 0.5f;
} debug;

Scene::Scene()
{
    suzanne = std::make_unique<ew::Model>("assets/models/suzanne.obj");
    blinnphong = std::make_unique<ew::Shader>("assets/shaders/blinnphong.vs", "assets/shaders/blinnphong.fs");
    texture = std::make_unique<ew::Texture>("assets/ornament-color.jpg");
    normalmap = std::make_unique<ew::Texture>("assets/ornament-normal.jpg");

    light = {
        .brightness = 1.0f,
        .color = {1.0f, 1.0f, 1.0f},
        .position = {0.0f, 2.0f, 0.0f},
    };
}

Scene::~Scene()
{
}

void Scene::Update(float dt)
{
    batteries::Scene::Update(dt);
}

auto matrix = glm::mat4(1.0f);

void Scene::Render(void)
{
    const auto view_proj = camera.Projection() * camera.View();

    glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);

    glBindTextureUnit(0, texture->getID());
    glBindTextureUnit(1, normalmap->getID()); 

    blinnphong->use();

    blinnphong->setInt("texture0", 0);

    blinnphong->setMat4("model", matrix);
    blinnphong->setMat4("view_proj", view_proj);
    blinnphong->setVec3("camera_position", camera.position);

    blinnphong->setVec3("light.position", light.position);
    blinnphong->setVec3("light.color", light.color);

    blinnphong->setFloat("material.shininess", debug.shininess);
    blinnphong->setVec3("material.diffuse", glm::vec3(debug.kd));
    blinnphong->setVec3("material.specular", glm::vec3(debug.ks));
    blinnphong->setVec3("material.ambient", glm::vec3(backgroundColor) * 0.5f);
    blinnphong->setInt("normal_map", 1);

    suzanne->draw();
}

void Scene::Debug(void)
{
    ImGuizmo::BeginFrame();
    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
    ImGuizmo::SetRect(0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);

    glm::mat4 m{1.0f};
    auto *view = glm::value_ptr(camera.View());
    auto *proj = glm::value_ptr(camera.Projection());

    if (ImGuizmo::IsUsing()) {
        light.position = glm::vec3(lightMatrix[3]);
    }

    ImGuizmo::Manipulate(
        view,
        proj,
        ImGuizmo::TRANSLATE,
        ImGuizmo::WORLD,
        glm::value_ptr(lightMatrix)
    );

    cameracontroller.Debug();

    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Checkbox("Paused", &time.paused);
    ImGui::SliderFloat("Time Factor", &time.factor, 0.0f, 10.0f);

    ImGui::ColorEdit3("Light Color", &light.color.x);

    ImGui::SliderFloat("Diffuse (Kd)", &debug.kd, 0.0f, 1.0f);
    ImGui::SliderFloat("Specular (Ks)", &debug.ks, 0.0f, 1.0f);
    ImGui::SliderFloat("Shininess", &debug.shininess, 2.0f, 1024.0f);

    ImGui::End();
}
