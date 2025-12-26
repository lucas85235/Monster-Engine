#include "MapEditorLayer.h"

#include <glad/glad.h>
#include <ImGuizmo.h>
#include <imgui.h>

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/input/InputManager.h"

namespace mst {

MapEditorLayer::MapEditorLayer() : Layer("MapEditorLayer") {}

MapEditorLayer::~MapEditorLayer() = default;

void MapEditorLayer::OnAttach() {
    Layer::OnAttach();
    SE_LOG_INFO("MapEditorLayer::OnAttach");
    
    context_ = CreateScope<EditorContext>();
    
    viewportPanel_ = CreateScope<ViewportPanel>();
    statusBarPanel_ = CreateScope<StatusBarPanel>();
    
    SetupEventHandlers();
    
    SE_LOG_INFO("MapEditorLayer: Initialized with new architecture");
}

void MapEditorLayer::OnDetach() {
    SE_LOG_INFO("MapEditorLayer::OnDetach");
    
    viewportPanel_.reset();
    statusBarPanel_.reset();
    
    se::Application::Get().SetActiveScene(nullptr);
    context_.reset();
    
    Layer::OnDetach();
}

void MapEditorLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);
    
    ProcessCameraInput(ts);
    
    inputHandler_.ProcessShortcuts(*context_, 
        viewportPanel_->IsHovered(), 
        viewportPanel_->IsFocused());
    
    context_->GetCamera().Update(ts);
    context_->GetScene().OnUpdate(ts);
}

void MapEditorLayer::OnRender() {
    Layer::OnRender();
    
    auto& renderer = viewportPanel_->GetRenderer();
    auto& camera = context_->GetCamera();
    auto& scene = context_->GetScene();
    
    renderer.BeginFrame();
    renderer.RenderGrid(camera);
    renderer.RenderScene(camera, scene);
    renderer.RenderColliderDebug(camera, scene);
    renderer.EndFrame();
}

void MapEditorLayer::OnImGuiRender() {
    Layer::OnImGuiRender();
    ImGuizmo::BeginFrame();
    
    SetupDockspace();
    
    MenuBarActions menuActions = menuBar_.Render();
    ProcessMenuActions(menuActions);
    
    ImGui::End();  // End dockspace
    
    hierarchyPanel_.Render(context_->GetScene(), context_->GetSelection());
    ProcessHierarchyActions();
    
    propertiesPanel_.Render(context_->GetSelection(), context_->GetGizmo());
    
    viewportPanel_->Render(*context_);
    
    statusBarPanel_->SetGridVisible(viewportPanel_->GetRenderer().GetSettings().showGrid);
    statusBarPanel_->SetColliderDebugVisible(viewportPanel_->GetRenderer().GetSettings().showColliderDebug);
    statusBarPanel_->Render(*context_);
    
    fileDialogs_.Render(*context_);
}

void MapEditorLayer::SetupEventHandlers() {
    auto& eventBus = context_->GetEventBus();
    
    eventBus.Subscribe<EntityCreatedEvent>([this](const EntityCreatedEvent& e) {
        context_->GetDocument().MarkDirty();
    });
    
    eventBus.Subscribe<EntityDeletedEvent>([this](const EntityDeletedEvent& e) {
        context_->GetDocument().MarkDirty();
    });
}

void MapEditorLayer::SetupDockspace() {
    ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    ImGui::Begin("DockSpace", nullptr, windowFlags);
    ImGui::PopStyleVar(3);
    
    ImGuiID dockspaceId = ImGui::GetID("MapEditorDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
}

void MapEditorLayer::ProcessCameraInput(float ts) {
    if (!viewportPanel_->IsHovered()) return;
    if (context_->GetGizmo().IsUsing()) return;
    
    auto& input = se::InputManager::Get();
    
    float mouseX = input.GetMousePosition().x;
    float mouseY = input.GetMousePosition().y;
    float dx = mouseX - lastMouseX_;
    float dy = mouseY - lastMouseY_;
    lastMouseX_ = mouseX;
    lastMouseY_ = mouseY;
    
    bool altPressed = input.IsKeyDown(GLFW_KEY_LEFT_ALT) || input.IsKeyDown(GLFW_KEY_RIGHT_ALT);
    bool leftButton = input.IsMouseButtonDown(0);
    bool middleButton = input.IsMouseButtonDown(2);
    bool rightButton = input.IsMouseButtonDown(1);
    
    bool orbiting = (altPressed && leftButton) || rightButton;
    bool panning = middleButton || (altPressed && middleButton);
    
    auto& camera = context_->GetCamera();
    
    if (orbiting) {
        camera.OnMouseMove(dx, dy, false, false, true);
    } else if (panning) {
        camera.OnMouseMove(dx, dy, false, true, false);
    }
    
    float scroll = ImGui::GetIO().MouseWheel;
    if (scroll != 0.0f) {
        camera.OnMouseScroll(scroll);
    }
}

void MapEditorLayer::ProcessMenuActions(const MenuBarActions& actions) {
    auto& entityManager = context_->GetEntityManager();
    auto& selection = context_->GetSelection();
    auto& document = context_->GetDocument();
    
    if (actions.newMap) {
        selection.ClearSelection();
        document.New();
        context_->GetCommandSystem().Clear();
    }
    
    if (actions.openMap) {
        fileDialogs_.ShowOpenDialog([this](const FileDialogResult& result) {
            if (result.confirmed) {
                context_->GetSelection().ClearSelection();
                context_->GetDocument().Open(result.filename);
                context_->GetCommandSystem().Clear();
            }
        });
    }
    
    if (actions.exportMap) {
        fileDialogs_.ShowExportDialog([this](const FileDialogResult& result) {
            if (result.confirmed) {
                context_->GetDocument().SetMapName(result.filename);
                context_->GetDocument().SaveAs(result.filename);
            }
        });
    }
    
    if (actions.exitApp) {
        se::Application::Get().Close();
    }
    
    if (actions.createCube) {
        auto entity = entityManager.CreatePrimitive(PrimitiveType::Cube);
        selection.Select(entity);
    }
    if (actions.createSphere) {
        auto entity = entityManager.CreatePrimitive(PrimitiveType::Sphere);
        selection.Select(entity);
    }
    if (actions.createCapsule) {
        auto entity = entityManager.CreatePrimitive(PrimitiveType::Capsule);
        selection.Select(entity);
    }
    if (actions.createCylinder) {
        auto entity = entityManager.CreatePrimitive(PrimitiveType::Cylinder);
        selection.Select(entity);
    }
    if (actions.createPlane) {
        auto entity = entityManager.CreatePrimitive(PrimitiveType::Plane);
        selection.Select(entity);
    }
    
    if (actions.createPlayerStart) {
        auto entity = entityManager.CreatePlayerStart();
        if (entity.IsValid()) {
            selection.Select(entity);
        }
    }
    
    if (actions.deleteSelected && selection.HasSelection()) {
        auto entities = selection.GetSelectedEntities();
        selection.ClearSelection();
        entityManager.DeleteEntities(entities);
    }
    
    if (actions.duplicateSelected && selection.HasSelection()) {
        auto entity = selection.GetPrimarySelection();
        auto duplicate = entityManager.DuplicateEntity(entity);
        if (duplicate.IsValid()) {
            selection.Select(duplicate);
        }
    }
    
    if (actions.toggleGrid) {
        viewportPanel_->ToggleGrid();
    }
    
    if (actions.toggleColliderDebug) {
        viewportPanel_->ToggleColliderDebug();
    }
    
    if (actions.resetCamera) {
        context_->GetCamera().FocusOnPoint({0.0f, 0.0f, 0.0f});
    }
}

void MapEditorLayer::ProcessHierarchyActions() {
    auto& selection = context_->GetSelection();
    auto& entityManager = context_->GetEntityManager();
    
    if (hierarchyPanel_.WantsDelete() && selection.HasSelection()) {
        auto entities = selection.GetSelectedEntities();
        selection.ClearSelection();
        entityManager.DeleteEntities(entities);
    }
    
    if (hierarchyPanel_.WantsDuplicate() && selection.HasSelection()) {
        auto entity = selection.GetPrimarySelection();
        auto duplicate = entityManager.DuplicateEntity(entity);
        if (duplicate.IsValid()) {
            selection.Select(duplicate);
        }
    }
    
    hierarchyPanel_.ClearActions();
}

}  // namespace mst
