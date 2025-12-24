#include "MapEditorLayer.h"

#include <ImGuizmo.h>
#include <imgui.h>
#include <gtc/type_ptr.hpp>

#include "engine/Application.h"
#include "engine/Log.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/input/InputManager.h"

namespace mst {

MapEditorLayer::MapEditorLayer() : Layer("MapEditorLayer") {
    currentMap_.mapName = "Untitled Map";
}

MapEditorLayer::~MapEditorLayer() = default;

void MapEditorLayer::OnAttach() {
    Layer::OnAttach();
    SE_LOG_INFO("MapEditorLayer::OnAttach");
    scene_ = CreateScope<se::Scene>("Editor Scene", se::SceneSettings{.EnablePhysics = false});
    se::Application::Get().SetActiveScene(scene_.get());
    editorCamera_.FocusOnPoint({0.0f, 0.0f, 0.0f});
    editorCamera_.SetOrbitDistance(15.0f);
}

void MapEditorLayer::OnDetach() {
    SE_LOG_INFO("MapEditorLayer::OnDetach");
    se::Application::Get().SetActiveScene(nullptr);
    scene_.reset();
    Layer::OnDetach();
}

void MapEditorLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);
    auto& input = se::InputManager::Get();
    float mouseX = input.GetMousePosition().x;
    float mouseY = input.GetMousePosition().y;
    float dx = mouseX - lastMouseX_;
    float dy = mouseY - lastMouseY_;
    lastMouseX_ = mouseX;
    lastMouseY_ = mouseY;
    bool leftButton = input.IsMouseButtonDown(0);
    bool middleButton = input.IsMouseButtonDown(2);
    bool rightButton = input.IsMouseButtonDown(1);
    if (!gizmo_.IsUsing() && !ImGui::GetIO().WantCaptureMouse) {
        editorCamera_.OnMouseMove(dx, dy, leftButton, middleButton, rightButton);
    }
    editorCamera_.Update(ts);
    scene_->OnUpdate(ts);
}

void MapEditorLayer::OnRender() {
    Layer::OnRender();
    auto& window = se::Application::Get().GetWindow();
    float aspectRatio = static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight());
    scene_->OnRender(editorCamera_.GetCamera(), aspectRatio);
}

void MapEditorLayer::OnImGuiRender() {
    Layer::OnImGuiRender();
    ImGuizmo::BeginFrame();
    
    ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, windowFlags);
    ImGui::PopStyleVar(3);
    ImGuiID dockspaceId = ImGui::GetID("MapEditorDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
    
    MenuBarActions menuActions = menuBar_.Render();
    ProcessMenuActions(menuActions);
    ImGui::End();
    
    hierarchy_.Render(*scene_, selection_);
    ProcessHierarchyActions();
    properties_.Render(selection_, gizmo_);
    RenderViewport();
    RenderStatusBar();
    if (showExportDialog_) ShowExportDialog();
    ProcessKeyboardShortcuts();
}

void MapEditorLayer::ProcessMenuActions(const MenuBarActions& actions) {
    if (actions.newMap) {
        selection_.ClearSelection();
        scene_->Clear();
        currentMap_.Clear();
        currentMap_.mapName = "Untitled Map";
        SE_LOG_INFO("New map created");
    }
    if (actions.exportMap) showExportDialog_ = true;
    if (actions.exitApp) se::Application::Get().Close();
    if (actions.createCube) CreatePrimitive(PrimitiveType::Cube);
    if (actions.createSphere) CreatePrimitive(PrimitiveType::Sphere);
    if (actions.createCapsule) CreatePrimitive(PrimitiveType::Capsule);
    if (actions.createCylinder) CreatePrimitive(PrimitiveType::Cylinder);
    if (actions.createPlane) CreatePrimitive(PrimitiveType::Plane);
    if (actions.deleteSelected) DeleteSelected();
    if (actions.duplicateSelected) DuplicateSelected();
    if (actions.toggleGrid) showGrid_ = !showGrid_;
    if (actions.resetCamera) {
        editorCamera_.FocusOnPoint({0.0f, 0.0f, 0.0f});
        editorCamera_.SetOrbitDistance(15.0f);
    }
}

void MapEditorLayer::ProcessHierarchyActions() {
    if (hierarchy_.WantsDelete()) DeleteSelected();
    if (hierarchy_.WantsDuplicate()) DuplicateSelected();
    hierarchy_.ClearActions();
}

void MapEditorLayer::ProcessKeyboardShortcuts() {
    if (ImGui::GetIO().WantCaptureKeyboard) return;
    auto& input = se::InputManager::Get();
    if (input.IsKeyDown(GLFW_KEY_W)) gizmo_.SetOperation(GizmoController::Operation::Translate);
    if (input.IsKeyDown(GLFW_KEY_E)) gizmo_.SetOperation(GizmoController::Operation::Rotate);
    if (input.IsKeyDown(GLFW_KEY_R)) gizmo_.SetOperation(GizmoController::Operation::Scale);
    if (input.IsKeyDown(GLFW_KEY_Q)) gizmo_.ToggleSpace();
    if (input.IsKeyDown(GLFW_KEY_DELETE)) DeleteSelected();
    if (input.IsKeyDown(GLFW_KEY_LEFT_CONTROL) && input.IsKeyDown(GLFW_KEY_D)) DuplicateSelected();
    if (input.IsKeyDown(GLFW_KEY_F) && selection_.HasSelection()) {
        auto entity = selection_.GetPrimarySelection();
        auto& transform = entity.GetComponent<se::TransformComponent>();
        editorCamera_.FocusOnPoint(transform.Position);
    }
    if (input.IsKeyDown(GLFW_KEY_G)) showGrid_ = !showGrid_;
    if (input.IsKeyDown(GLFW_KEY_LEFT_CONTROL) && input.IsKeyDown(GLFW_KEY_E)) showExportDialog_ = true;
    if (input.IsKeyDown(GLFW_KEY_ESCAPE)) selection_.ClearSelection();
}

void MapEditorLayer::CreatePrimitive(PrimitiveType type) {
    se::Entity entity = PrimitiveFactory::CreatePrimitive(*scene_, type);
    selection_.Select(entity);
}

void MapEditorLayer::DuplicateSelected() {
    if (!selection_.HasSelection()) return;
    auto entity = selection_.GetPrimarySelection();
    if (!entity.IsValid()) return;
    auto& name = entity.GetComponent<se::NameComponent>().Name;
    auto& transform = entity.GetComponent<se::TransformComponent>();
    auto& metadata = entity.GetComponent<PrimitiveFactory::EditorMetadata>();
    se::Entity duplicate = PrimitiveFactory::CreatePrimitive(*scene_, metadata.primitiveType, name + "_copy");
    auto& dupTransform = duplicate.GetComponent<se::TransformComponent>();
    dupTransform.SetPosition(transform.Position + Vector3(1.0f, 0.0f, 0.0f));
    dupTransform.SetRotation(transform.Rotation);
    dupTransform.SetScale(transform.Scale);
    auto& dupMetadata = duplicate.GetComponent<PrimitiveFactory::EditorMetadata>();
    dupMetadata.hasCollision = metadata.hasCollision;
    dupMetadata.colliderType = metadata.colliderType;
    dupMetadata.colliderSize = metadata.colliderSize;
    dupMetadata.colliderRadius = metadata.colliderRadius;
    dupMetadata.colliderHeight = metadata.colliderHeight;
    if (entity.HasComponent<se::MeshRenderComponent>()) {
        auto& mesh = entity.GetComponent<se::MeshRenderComponent>();
        auto& dupMesh = duplicate.GetComponent<se::MeshRenderComponent>();
        dupMesh.Color = mesh.Color;
    }
    selection_.Select(duplicate);
    SE_LOG_INFO("Duplicated entity '{}' -> '{}'", name, name + "_copy");
}

void MapEditorLayer::DeleteSelected() {
    if (!selection_.HasSelection()) return;
    auto entities = selection_.GetSelectedEntities();
    selection_.ClearSelection();
    for (auto entity : entities) {
        if (entity.IsValid()) {
            auto& name = entity.GetComponent<se::NameComponent>().Name;
            SE_LOG_INFO("Deleting entity '{}'", name);
            scene_->DestroyEntity(entity);
        }
    }
}

void MapEditorLayer::ShowExportDialog() {
    ImGui::OpenPopup("Export Map");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Export Map", &showExportDialog_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Export map as .mstmap file");
        ImGui::Separator();
        ImGui::InputText("File Name", exportFileName_, sizeof(exportFileName_));
        ImGui::Separator();
        if (ImGui::Button("Export", ImVec2(120, 0))) {
            ExportMap(std::string(exportFileName_) + ".mstmap");
            showExportDialog_ = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) showExportDialog_ = false;
        ImGui::EndPopup();
    }
}

void MapEditorLayer::ExportMap(const std::string& filename) {
    BuildMapData();
    currentMap_.mapName = exportFileName_;
    std::filesystem::path path = filename;
    if (MapSerializer::Export(currentMap_, path)) {
        SE_LOG_INFO("Map exported successfully to '{}'", filename);
    } else {
        SE_LOG_ERROR("Failed to export map to '{}'", filename);
    }
}

void MapEditorLayer::BuildMapData() {
    currentMap_.entities.clear();
    auto view = scene_->GetAllEntitiesWith<se::NameComponent, se::TransformComponent, PrimitiveFactory::EditorMetadata>();
    for (auto entityHandle : view) {
        se::Entity entity(entityHandle, scene_.get());
        MapEntityData data;
        data.name = entity.GetComponent<se::NameComponent>().Name;
        auto& transform = entity.GetComponent<se::TransformComponent>();
        data.position = transform.Position;
        data.rotation = transform.Rotation;
        data.scale = transform.Scale;
        auto& metadata = entity.GetComponent<PrimitiveFactory::EditorMetadata>();
        data.primitiveType = metadata.primitiveType;
        data.hasCollision = metadata.hasCollision;
        data.colliderType = metadata.colliderType;
        data.colliderSize = metadata.colliderSize;
        data.colliderRadius = metadata.colliderRadius;
        data.colliderHeight = metadata.colliderHeight;
        if (entity.HasComponent<se::MeshRenderComponent>()) {
            data.color = entity.GetComponent<se::MeshRenderComponent>().Color;
        }
        currentMap_.entities.push_back(data);
    }
    SE_LOG_INFO("Built map data with {} entities", currentMap_.entities.size());
}

void MapEditorLayer::RenderGrid() {
    if (!showGrid_) return;
    auto& window = se::Application::Get().GetWindow();
    float aspectRatio = static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight());
    Matrix4 view = editorCamera_.GetCamera().getViewMatrix();
    Matrix4 projection = editorCamera_.GetCamera().getProjectionMatrix(aspectRatio);
    Matrix4 identity = Matrix4(1.0f);
    ImGuizmo::DrawGrid(glm::value_ptr(view), glm::value_ptr(projection), glm::value_ptr(identity), 100.0f);
}

void MapEditorLayer::RenderViewport() {
    ImGui::Begin("Viewport");
    auto& window = se::Application::Get().GetWindow();
    float aspectRatio = static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight());
    RenderGrid();
    if (selection_.HasSelection()) {
        auto entity = selection_.GetPrimarySelection();
        if (entity.IsValid() && entity.HasComponent<se::TransformComponent>()) {
            auto& transform = entity.GetComponent<se::TransformComponent>();
            gizmo_.Manipulate(editorCamera_.GetCamera(), aspectRatio, transform);
        }
    }
    ImGui::End();
}

void MapEditorLayer::RenderStatusBar() {
    ImGui::Begin("Status Bar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("Entities: %zu", scene_->GetEntityCount());
    ImGui::SameLine(); ImGui::Text(" | ");
    ImGui::SameLine(); ImGui::Text("Selected: %zu", selection_.GetSelectedEntities().size());
    ImGui::SameLine(); ImGui::Text(" | ");
    ImGui::SameLine(); ImGui::Text("Grid: %s", showGrid_ ? "ON" : "OFF");
    ImGui::SameLine(); ImGui::Text(" | ");
    ImGui::SameLine(); ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
    ImGui::End();
}

}  // namespace mst
