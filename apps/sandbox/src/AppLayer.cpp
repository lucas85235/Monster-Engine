#include "AppLayer.h"
#include <GLFW/glfw3.h>
#include <engine/Application.h>
#include <engine/Input.h>
#include <engine/Log.h>
#include <engine/ecs/Components.h>
#include <gtc/type_ptr.hpp>
#include <imgui.h>

AppLayer::AppLayer()
    : Layer("AppLayer"), camera_(glm::vec3(0.0f, 0.0f, 10.0f)), inputHandler_(camera_) {}

AppLayer::~AppLayer() {}

void AppLayer::OnAttach() {
    SE_LOG_INFO("AppLayer attached");

    // Material load
    LoadMaterial();

    // Create scene
    scene_ = std::make_unique<se::Scene>("Main Scene");

    AddDirectionalLight();

    // Create original entities
    CreateCubeEntity("Cube", {0.0f, -2.0f, 0.0f}, {50.0f, 1.0f, 50.0f});
    CreateCubeEntity("Rotating Cube", {3.0f, 0.0f, -2.0f});
    CreateSphereEntity("Sphere", {-3.0f, 0.0f, -2.0f});
    CreateCapsuleEntity("Capsule", {0.0f, 2.5f, -2.0f});

    SE_LOG_INFO("Scene setup complete with {} entities", scene_->GetEntityCount());

    auto& app = se::Application::Get();
    auto* window = app.GetWindow().GetNativeWindow();

    if (window) {
        inputHandler_.initialize(window);
    }

    InputHandler::setCursorModeFromString(window, "normal");

    se::RenderCommand::SetClearColor({0.3f, 0.3f, 0.3f, 1.0f});

    // Audio test
    CreateAudioEntity("Audio", {0.0f, 0.0f, 0.0f});
}

void AppLayer::LoadMaterial() {
    auto assets_folder = findAssetsFolder();
    fs::path fragment_shader_location = assets_folder.value() / "shaders" / "basic.frag";
    fs::path vertex_shader_location = assets_folder.value() / "shaders" / "basic.vert";

    std::shared_ptr<se::Shader> shader = se::MaterialManager::GetShader(
        "DefaultShader", vertex_shader_location, fragment_shader_location);

    material_ = se::MaterialManager::CreateMaterial(shader);
    material_->SetFloat("uSpecularStrength", 0.5f);
}

void AppLayer::OnDetach() {
    SE_LOG_INFO("AppLayer detached");
    scene_.reset();
}

void AppLayer::OnEvent(se::Event& event) {
    // Handle events if needed
}

void AppLayer::OnUpdate(float ts) {
    animationTime_ += ts;

    // Update scene systems
    scene_->OnUpdate(ts);

    // Update entity transforms
    auto view = scene_->GetAllEntitiesWith<se::TransformComponent, se::NameComponent>();
    for (auto entity : view) {
        auto& transform = view.get<se::TransformComponent>(entity);
        auto& name = view.get<se::NameComponent>(entity);

        // Rotate specific entities
        if (name.Name == "Rotating Cube") {
            transform.Rotate({0.0f, 50.0f * ts, 0.0f});
        }

        // Make sphere bounce
        if (name.Name == "Sphere") {
            float bounce = glm::sin(animationTime_ * 2.0f) * 0.5f;
            transform.Position.y = bounce;
        }

        // Make capsule rotate on X axis
        if (name.Name == "Capsule") {
            transform.Rotate({0.0f, 30.0f * ts, 0.0f});
            transform.SetScale(glm::vec3(1.0, 1.0, 1.0) * glm::sin(animationTime_ * 2) * 0.5f +
                               1.0f);
        }
    }

    // Handle input
    HandleInput(ts);

    AudioSystem::GetAudioSystem()->SetListenerTransform(camera_.GetPosition());
    AudioSystem::GetAudioSystem()->SetListenerForward(camera_.GetFront());
    AudioSystem::GetAudioSystem()->SetListenerUp(camera_.GetUp());
}

void AppLayer::OnRender() {
    // Calculate aspect ratio
    glm::vec2 windowSize = {se::Application::Get().GetWindow().GetWidth(),
                            se::Application::Get().GetWindow().GetHeight()};
    float aspectRatio = windowSize.x / windowSize.y;

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
        auto view = scene_->GetAllEntitiesWith<se::NameComponent, se::TransformComponent>();

        for (auto entity : view) {
            auto& name = view.get<se::NameComponent>(entity);
            auto& transform = view.get<se::TransformComponent>(entity);

            ImGui::PushID(static_cast<int>(entity));

            if (ImGui::TreeNode(name.Name.c_str())) {
                ImGui::Text("ID: %u", static_cast<uint32_t>(entity));

                // Transform controls
                ImGui::DragFloat3("Position", glm::value_ptr(transform.Position), 0.1f);
                ImGui::DragFloat3("Rotation", glm::value_ptr(transform.Rotation), 1.0f);
                ImGui::DragFloat3("Scale", glm::value_ptr(transform.Scale), 0.1f);

                // Mesh render component controls
                se::Entity ent(entity, scene_.get());
                if (ent.HasComponent<se::MeshRenderComponent>()) {
                    auto& meshRender = ent.GetComponent<se::MeshRenderComponent>();

                    ImGui::Separator();
                    ImGui::Checkbox("Visible", &meshRender.IsVisible);
                    ImGui::Checkbox("Cast Shadows", &meshRender.CastShadows);
                    ImGui::Checkbox("Receive Shadows", &meshRender.ReceiveShadows);
                }
                if (ent.HasComponent<se::DirectionalLightComponent>()) {
                    auto& light = ent.GetComponent<se::DirectionalLightComponent>();

                    ImGui::Separator();
                    ImGui::Text("Directional Light");
                    ImGui::Checkbox("Enabled", &light.Enabled);
                    ImGui::Checkbox("Cast Shadows##Light", &light.CastShadows);
                    ImGui::DragFloat("Intensity", &light.Intensity, 0.05f, 0.0f, 20.0f);
                    ImGui::ColorEdit3("Color", glm::value_ptr(light.Color));
                    light.Intensity = glm::max(light.Intensity, 0.0f);
                }
                if (ent.HasComponent<se::AudioComponent>()) {
                    auto& audio = ent.GetComponent<se::AudioComponent>();

                    ImGui::Separator();
                    ImGui::Text("Audio source");
                    
                    ImGui::Text("File path: %s", audio.FilePath.c_str());
                    ImGui::Checkbox("Play in a loop", &audio.IsLooping);
                    ImGui::DragFloat("Volume", &audio.Volume, 0.01f, 0.0f, 1.0f);
                    ImGui::InputFloat("Radius", &audio.Radius);
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }
    }

    ImGui::Separator();

    // Render stats
    auto stats = se::Application::Get().GetRenderer().GetStats();
    if (ImGui::CollapsingHeader("Render Stats")) {
        ImGui::Text("Draw Calls: %u", stats.DrawCalls);
        ImGui::Text("Triangles: %u", stats.TriangleCount);
    }

    ImGui::Separator();

    // Quick actions
    if (ImGui::CollapsingHeader("Quick Actions")) {
        if (ImGui::Button("Add Cube")) {
            static int cubeCount = 0;
            float x = (rand() % 10 - 5) * 0.5f;
            float y = (rand() % 10 - 5) * 0.5f;
            CreateCubeEntity("Cube_" + std::to_string(cubeCount++), {x, y, -5.0f});
        }

        ImGui::SameLine();

        if (ImGui::Button("Add Sphere")) {
            static int sphereCount = 0;
            float x = (rand() % 10 - 5) * 0.5f;
            float y = (rand() % 10 - 5) * 0.5f;
            CreateSphereEntity("Sphere_" + std::to_string(sphereCount++), {x, y, -5.0f});
        }

        if (ImGui::Button("Add DirectionalLight")) {
            AddDirectionalLight();
        }

        if (ImGui::Button("Clear Scene")) {
            scene_->Clear();
            SE_LOG_INFO("Scene cleared");
        }
    }

    ImGui::End();
}

void AppLayer::HandleInput(float deltaTime) {
    auto& app = se::Application::Get();
    GLFWwindow* window = app.GetWindow().GetNativeWindow();

    if (window) {
        inputHandler_.processKeyboard(window, deltaTime);
    }

    if (se::Input::IsKeyPressed(GLFW_KEY_TAB)) {
        camera_active_ = !camera_active_;
        camera_.SetActive(camera_active_);
    }
}

// ==================== Entity Creation Helpers ====================

void AppLayer::CreateCubeEntity(const std::string& name, const glm::vec3& position,
                                const glm::vec3& scale) {
    SE_LOG_INFO("Creating cube entity: {}", name);

    auto entity = scene_->CreateEntity(name);

    auto mesh = se::MeshManager::GetPrimitive(se::PrimitiveMeshType::Cube);

    if (!mesh) {
        SE_LOG_ERROR("Failed to get cube mesh!");
        return;
    }

    // Add mesh render component - Engine handles everything!
    entity.AddComponent<se::MeshRenderComponent>(mesh, material_);

    // Set position
    auto& transform = entity.GetComponent<se::TransformComponent>();
    transform.SetPosition(position);
    transform.SetScale(scale);

    SE_LOG_INFO("Cube entity created successfully at ({}, {}, {})", position.x, position.y,
                position.z);
}

void AppLayer::AddDirectionalLight() {
    auto sunEntity = scene_->CreateEntity("Sun Light");
    auto& sunTransform = sunEntity.GetComponent<se::TransformComponent>();
    sunTransform.SetPosition({0.0f, 5.0f, 5.0f});
    sunTransform.SetRotation({100.0f, 0.0f, 0.0f});

    auto mesh = se::MeshManager::GetPrimitive(se::PrimitiveMeshType::Cube);

    auto& sunMesh = sunEntity.AddComponent<se::MeshRenderComponent>(mesh, material_);

    auto& sunLight = sunEntity.AddComponent<se::DirectionalLightComponent>();
    sunLight.Color = {1.0f, 0.98f, 0.9f};
    sunLight.Intensity = 1.5f;
}

void AppLayer::CreateSphereEntity(const std::string& name, const glm::vec3& position) {
    SE_LOG_INFO("Creating sphere entity: {}", name);

    auto entity = scene_->CreateEntity(name);

    // Add mesh render component
    entity.AddComponent<se::MeshRenderComponent>(
        se::MeshManager::GetPrimitive(se::PrimitiveMeshType::Sphere), material_);

    // Set position
    auto& transform = entity.GetComponent<se::TransformComponent>();
    transform.SetPosition(position);

    SE_LOG_INFO("Sphere entity created successfully");
}

void AppLayer::CreateCapsuleEntity(const std::string& name, const glm::vec3& position) {
    SE_LOG_INFO("Creating capsule entity: {}", name);

    auto entity = scene_->CreateEntity(name);

    // Add mesh render component
    entity.AddComponent<se::MeshRenderComponent>(
        se::MeshManager::GetPrimitive(se::PrimitiveMeshType::Sphere), material_);

    // Set position and scale
    auto& transform = entity.GetComponent<se::TransformComponent>();
    transform.SetPosition(position);
    transform.SetScale({0.5f, 0.5f, 0.5f});

    SE_LOG_INFO("Capsule entity created successfully");
}

void AppLayer::CreateAudioEntity(const std::string& name, const glm::vec3& position) {
    SE_LOG_INFO("OpenAL: Set listener position");
    AudioSystem::GetAudioSystem()->SetListenerTransform(camera_.GetPosition());

    SE_LOG_INFO("OpenAL: Creating audio entity: {}", name);
    auto entity = scene_->CreateEntity(name);
    

    // Set position and scale
    auto& transform = entity.GetComponent<se::TransformComponent>();
    transform.SetPosition(position);

    SE_LOG_INFO("OpenAL: Audio entity created successfully");

    auto assets_folder = findAssetsFolder();
    fs::path music_path = assets_folder.value() / "audio" / "music.wav";

    auto& audioComponent = entity.AddComponent<se::AudioComponent>(music_path);
    audioComponent.PrepareAndPlay();
}
