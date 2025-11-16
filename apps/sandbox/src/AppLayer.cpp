#include "AppLayer.h"

#include <GLFW/glfw3.h>
#include <engine/Application.h>
#include <engine/Log.h>
#include <engine/ecs/Components.h>
#include <engine/input/Input.h>
#include <imgui.h>

#include <gtc/type_ptr.hpp>

#include "SampleUtilities.h"
#include "engine/event/KeyEvent.h"
#include "engine/event/MouseEvent.h"

AppLayer::AppLayer() : Layer("AppLayer"), camera_(glm::vec3(0.0f, 0.0f, 10.0f)), inputHandler_(camera_) {}

AppLayer::~AppLayer() {}

void AppLayer::OnAttach() {
    SE_LOG_INFO("AppLayer attached");

    // Material load
    material_ = Utilities::LoadMaterial();

    // Create scene
    scene_ = CreateScope<Scene>("Main Scene");

    AddDirectionalLight();

    // Create original entities
    Utilities::CreateCubeEntity("Cube", {0.0f, -2.0f, 0.0f}, {50.0f, 1.0f, 50.0f}, scene_.get(), material_);
    CreateCubeEntity("Rotating Cube", {3.0f, 0.0f, -2.0f});
    CreateSphereEntity("Sphere", {-3.0f, 0.0f, -2.0f});
    CreateCapsuleEntity("Capsule", {0.0f, 2.5f, -2.0f});

    SE_LOG_INFO("Scene setup complete with {} entities", scene_->GetEntityCount());

    auto& app    = Application::Get();
    auto* window = app.GetWindow().GetNativeWindow();

    if (window) { inputHandler_.initialize(window); }

    InputHandler::setCursorModeFromString(window, "normal");

    RenderCommand::SetClearColor({0.3f, 0.3f, 0.3f, 1.0f});
}

void AppLayer::OnDetach() {
    SE_LOG_INFO("AppLayer detached");
    scene_.reset();
}

void AppLayer::OnEvent(Event& event) {
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<KeyPressedEvent>(SE_BIND_EVENT_FN(OnKeyPressedEvent));
    dispatcher.Dispatch<MouseButtonPressedEvent>(SE_BIND_EVENT_FN(OnMouseButtonPressed));
}

bool AppLayer::OnKeyPressedEvent(KeyPressedEvent& e) {
    // with a modifier logic and no repeat
    if (e.GetRepeatCount() == 0) {
        switch (e.GetKeyCode()) {
            case Key::Tab:
                camera_active_ = !camera_active_;
                camera_.SetActive(camera_active_);
                break;
        }
    }

    return false;
}

bool AppLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e) {
    return false;
}

void AppLayer::OnUpdate(float ts) {
    animationTime_ += ts;

    // Update scene systems
    scene_->OnUpdate(ts);

    // Update entity transforms
    auto view = scene_->GetAllEntitiesWith<TransformComponent, NameComponent>();
    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        auto& name      = view.get<NameComponent>(entity);

        // Rotate specific entities
        if (name.Name == "Rotating Cube") { transform.Rotate({0.0f, 50.0f * ts, 0.0f}); }

        // Make sphere bounce
        if (name.Name == "Sphere") {
            float bounce         = glm::sin(animationTime_ * 2.0f) * 0.5f;
            transform.Position.y = bounce;
        }

        // Make capsule rotate on X axis
        if (name.Name == "Capsule") {
            transform.Rotate({0.0f, 30.0f * ts, 0.0f});
            transform.SetScale(glm::vec3(1.0, 1.0, 1.0) * glm::sin(animationTime_ * 2) * 0.5f + 1.0f);
        }
    }

    // Handle input
    HandleInput(ts);
}

void AppLayer::OnRender() {
    // Calculate aspect ratio
    glm::vec2 windowSize  = {Application::Get().GetWindow().GetWidth(), Application::Get().GetWindow().GetHeight()};
    float     aspectRatio = windowSize.x / windowSize.y;

    // Scene automatically renders all entities with MeshRenderComponent!
    scene_->OnRender(camera_, aspectRatio);
}

void AppLayer::OnImGuiRender() {
    ImGui::Begin("Scene Inspector");

    // Scene info
    ImGui::Text("Scene: %s", scene_->GetName().c_str());
    ImGui::Text("Entity Count: %zu", scene_->GetEntityCount());

    ImGui::Separator();

    // Camera info
    if (ImGui::CollapsingHeader("Camera")) {
        glm::vec3 camPos = camera_.GetPosition();
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", camPos.x, camPos.y, camPos.z);
        ImGui::Text("Yaw: %.2f | Pitch: %.2f", camera_.GetYaw(), camera_.GetPitch());
        ImGui::Text("Zoom: %.2f", camera_.GetZoom());
    }

    ImGui::Separator();

    // Entity list
    if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto view = scene_->GetAllEntitiesWith<NameComponent, TransformComponent>();

        for (auto entity : view) {
            auto& name      = view.get<NameComponent>(entity);
            auto& transform = view.get<TransformComponent>(entity);

            ImGui::PushID(static_cast<int>(entity));

            if (ImGui::TreeNode(name.Name.c_str())) {
                ImGui::Text("ID: %u", static_cast<uint32_t>(entity));

                // Transform controls
                ImGui::DragFloat3("Position", glm::value_ptr(transform.Position), 0.1f);
                ImGui::DragFloat3("Rotation", glm::value_ptr(transform.Rotation), 1.0f);
                ImGui::DragFloat3("Scale", glm::value_ptr(transform.Scale), 0.1f);

                // Mesh render component controls
                Entity ent(entity, scene_.get());
                if (ent.HasComponent<MeshRenderComponent>()) {
                    auto& meshRender = ent.GetComponent<MeshRenderComponent>();

                    ImGui::Separator();
                    ImGui::Checkbox("Visible", &meshRender.IsVisible);
                    ImGui::Checkbox("Cast Shadows", &meshRender.CastShadows);
                    ImGui::Checkbox("Receive Shadows", &meshRender.ReceiveShadows);
                }
                if (ent.HasComponent<DirectionalLightComponent>()) {
                    auto& light = ent.GetComponent<DirectionalLightComponent>();

                    ImGui::Separator();
                    ImGui::Text("Directional Light");
                    ImGui::Checkbox("Enabled", &light.Enabled);
                    ImGui::Checkbox("Cast Shadows##Light", &light.CastShadows);
                    ImGui::DragFloat("Intensity", &light.Intensity, 0.05f, 0.0f, 20.0f);
                    ImGui::ColorEdit3("Color", glm::value_ptr(light.Color));
                    light.Intensity = glm::max(light.Intensity, 0.0f);
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }
    }

    ImGui::Separator();

    // Render stats
    auto stats = Application::Get().GetRenderer().GetStats();
    if (ImGui::CollapsingHeader("Render Stats")) {
        ImGui::Text("Draw Calls: %u", stats.DrawCalls);
        ImGui::Text("Triangles: %u", stats.TriangleCount);
    }

    ImGui::Separator();

    // Quick actions
    if (ImGui::CollapsingHeader("Quick Actions")) {
        if (ImGui::Button("Add Cube")) {
            static int cubeCount = 0;
            float      x         = (rand() % 10 - 5) * 0.5f;
            float      y         = (rand() % 10 - 5) * 0.5f;
            CreateCubeEntity("Cube_" + std::to_string(cubeCount++), {x, y, -5.0f});
        }

        ImGui::SameLine();

        if (ImGui::Button("Add Sphere")) {
            static int sphereCount = 0;
            float      x           = (rand() % 10 - 5) * 0.5f;
            float      y           = (rand() % 10 - 5) * 0.5f;
            CreateSphereEntity("Sphere_" + std::to_string(sphereCount++), {x, y, -5.0f});
        }

        if (ImGui::Button("Add DirectionalLight")) { AddDirectionalLight(); }

        if (ImGui::Button("Clear Scene")) {
            scene_->Clear();
            SE_LOG_INFO("Scene cleared");
        }
    }

    ImGui::End();
}

void AppLayer::HandleInput(float deltaTime) {
    auto&       app    = Application::Get();
    GLFWwindow* window = app.GetWindow().GetNativeWindow();

    if (window) { inputHandler_.processKeyboard(window, deltaTime); }
}

// ==================== Entity Creation Helpers ====================

void AppLayer::CreateCubeEntity(const std::string& name, const glm::vec3& position, const glm::vec3& scale) {
    SE_LOG_INFO("Creating cube entity: {}", name);

    auto entity = scene_->CreateEntity(name);

    auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);

    if (!mesh) {
        SE_LOG_ERROR("Failed to get cube mesh!");
        return;
    }

    // Add mesh render component - Engine handles everything!
    entity.AddComponent<MeshRenderComponent>(mesh, material_);

    // Set position
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.SetPosition(position);
    transform.SetScale(scale);

    SE_LOG_INFO("Cube entity created successfully at ({}, {}, {})", position.x, position.y, position.z);
}

void AppLayer::AddDirectionalLight() {
    auto  sunEntity    = scene_->CreateEntity("Sun Light");
    auto& sunTransform = sunEntity.GetComponent<TransformComponent>();
    sunTransform.SetPosition({0.0f, 5.0f, 5.0f});
    sunTransform.SetRotation({100.0f, 0.0f, 0.0f});

    auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);

    auto& sunMesh = sunEntity.AddComponent<MeshRenderComponent>(mesh, material_);

    auto& sunLight     = sunEntity.AddComponent<DirectionalLightComponent>();
    sunLight.Color     = {1.0f, 0.98f, 0.9f};
    sunLight.Intensity = 1.5f;
}

void AppLayer::CreateSphereEntity(const std::string& name, const glm::vec3& position) {
    SE_LOG_INFO("Creating sphere entity: {}", name);

    auto entity = scene_->CreateEntity(name);

    // Add mesh render component
    entity.AddComponent<MeshRenderComponent>(MeshManager::GetPrimitive(PrimitiveMeshType::Sphere), material_);

    // Set position
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.SetPosition(position);

    SE_LOG_INFO("Sphere entity created successfully");
}

void AppLayer::CreateCapsuleEntity(const std::string& name, const glm::vec3& position) {
    SE_LOG_INFO("Creating capsule entity: {}", name);

    auto entity = scene_->CreateEntity(name);

    // Add mesh render component
    entity.AddComponent<MeshRenderComponent>(MeshManager::GetPrimitive(PrimitiveMeshType::Sphere), material_);

    // Set position and scale
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.SetPosition(position);
    transform.SetScale({0.5f, 0.5f, 0.5f});

    SE_LOG_INFO("Capsule entity created successfully");
}
