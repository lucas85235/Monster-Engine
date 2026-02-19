#include "engine/editor/EditorLayer.h"

#include <imgui.h>

#include "engine/Application.h"
#include "engine/Log.h"
#include "engine/core/ServiceLocator.h"
#include "engine/ecs/FilamentComponents.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/editor/EditorMenu.h"
#include "engine/editor/EditorPanel.h"
#include "engine/renderer/MaterialSystem.h"
#include "engine/renderer/MeshData.h"
#include "engine/renderer/MeshSystem.h"

// Panel includes (engine ships these by default)
#include "engine/editor/panels/InspectorPanel.h"
#include "engine/editor/panels/PerformancePanel.h"
#include "engine/editor/panels/SceneHierarchyPanel.h"

namespace se {

EditorLayer::EditorLayer() : Layer("EditorLayer") {}

EditorLayer::~EditorLayer() = default;

void EditorLayer::OnAttach() {
    SE_LOG_INFO("EditorLayer attached");

    // Register default panels
    AddPanel(std::make_unique<SceneHierarchyPanel>(this));
    AddPanel(std::make_unique<InspectorPanel>(this));
    AddPanel(std::make_unique<PerformancePanel>());

    // Register default entity creation menu items
    RegisterDefaultMenuItems();
}

void EditorLayer::OnDetach() {
    panels_.clear();
    EditorMenu::Clear();
    SE_LOG_INFO("EditorLayer detached");
}

void EditorLayer::SetScene(Scene* scene) {
    scene_ = scene;
    for (auto& panel : panels_) {
        panel->SetScene(scene);
    }
}

void EditorLayer::AddPanel(std::unique_ptr<EditorPanel> panel) {
    panel->SetScene(scene_);
    panels_.push_back(std::move(panel));
}

void EditorLayer::OnImGuiRender() {
    // Auto-discover scene from Application if not explicitly set
    if (!scene_) {
        Scene* activeScene = Application::Get().GetActiveScene();
        if (activeScene) {
            SetScene(activeScene);
        }
    }

    SetupDockSpace();

    // Render all open panels
    for (auto& panel : panels_) {
        if (panel->IsOpen()) {
            panel->OnImGuiRender();
        }
    }

    // ImGui Demo window (for development reference)
    if (showDemoWindow_) {
        ImGui::ShowDemoWindow(&showDemoWindow_);
    }
}

void EditorLayer::SetupDockSpace() {
    // Fullscreen dockspace
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpace", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    // Create the dockspace
    ImGuiID dockspaceId = ImGui::GetID("MonsterEngineDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    // Menu bar inside the dockspace window
    RenderMainMenuBar();

    ImGui::End();
}

void EditorLayer::RenderMainMenuBar() {
    if (ImGui::BeginMenuBar()) {
        // Registered menus (Entity creation, etc.)
        EditorMenu::RenderMenuItems(scene_);

        // View menu (panel toggles)
        RenderViewMenu();

        ImGui::EndMenuBar();
    }
}

void EditorLayer::RenderViewMenu() {
    if (ImGui::BeginMenu("View")) {
        for (auto& panel : panels_) {
            bool open = panel->IsOpen();
            if (ImGui::MenuItem(panel->GetName(), nullptr, &open)) {
                panel->SetOpen(open);
            }
        }

        ImGui::Separator();
        ImGui::MenuItem("ImGui Demo", nullptr, &showDemoWindow_);

        ImGui::EndMenu();
    }
}

void EditorLayer::RegisterDefaultMenuItems() {
    // ─── Entity / Primitives ─────────────────────────────────────
    EditorMenu::Register({"Empty Entity", "Entity", [](Scene& scene) {
        scene.CreateEntity("New Entity");
    }});

    EditorMenu::Register({"Cube", "Entity/Primitives", [](Scene& scene) {
        auto entity = scene.CreateEntity("Cube");
        auto& materials = ServiceLocator::Get().GetMaterialSystem();
        auto& meshes    = ServiceLocator::Get().GetMeshSystem();
        auto meshData   = MeshPrimitives::CreateBox();
        auto renderable = meshes.CreateRenderable(meshData, materials.GetDefaultLit());
        entity.AddComponent<FilamentRenderableComponent>(renderable);
    }});

    EditorMenu::Register({"Sphere", "Entity/Primitives", [](Scene& scene) {
        auto entity = scene.CreateEntity("Sphere");
        auto& materials = ServiceLocator::Get().GetMaterialSystem();
        auto& meshes    = ServiceLocator::Get().GetMeshSystem();
        auto meshData   = MeshPrimitives::CreateSphere();
        auto renderable = meshes.CreateRenderable(meshData, materials.GetDefaultLit());
        entity.AddComponent<FilamentRenderableComponent>(renderable);
    }});

    EditorMenu::Register({"Plane", "Entity/Primitives", [](Scene& scene) {
        auto entity = scene.CreateEntity("Plane");
        auto& materials = ServiceLocator::Get().GetMaterialSystem();
        auto& meshes    = ServiceLocator::Get().GetMeshSystem();
        auto meshData   = MeshPrimitives::CreatePlane(10.0f, 10.0f);
        auto renderable = meshes.CreateRenderable(meshData, materials.GetDefaultLit(), false);
        entity.AddComponent<FilamentRenderableComponent>(renderable);
    }});

    EditorMenu::Register({"Cylinder", "Entity/Primitives", [](Scene& scene) {
        auto entity = scene.CreateEntity("Cylinder");
        auto& materials = ServiceLocator::Get().GetMaterialSystem();
        auto& meshes    = ServiceLocator::Get().GetMeshSystem();
        auto meshData   = MeshPrimitives::CreateCylinder();
        auto renderable = meshes.CreateRenderable(meshData, materials.GetDefaultLit());
        entity.AddComponent<FilamentRenderableComponent>(renderable);
    }});

    // ─── Entity / Lights ─────────────────────────────────────────
    EditorMenu::Register({"Directional Light", "Entity/Lights", [](Scene& scene) {
        scene.CreateDirectionalLight("Directional Light");
    }});

    EditorMenu::Register({"Point Light", "Entity/Lights", [](Scene& scene) {
        auto entity = scene.CreatePointLight("Point Light");
        entity.GetComponent<TransformComponent>().SetPosition({0.0f, 3.0f, 0.0f});
    }});

    // ─── Entity / Camera ─────────────────────────────────────────
    EditorMenu::Register({"Camera (FreeFly)", "Entity/Camera", [](Scene& scene) {
        scene.CreateCamera("Camera", CameraMode::FreeFly);
    }});

    EditorMenu::Register({"Camera (Third Person)", "Entity/Camera", [](Scene& scene) {
        scene.CreateCamera("Camera", CameraMode::ThirdPerson);
    }});

    EditorMenu::Register({"Camera (Orbit)", "Entity/Camera", [](Scene& scene) {
        scene.CreateCamera("Camera", CameraMode::Orbit);
    }});
}

}  // namespace se
