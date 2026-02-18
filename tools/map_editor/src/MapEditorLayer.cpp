#include "MapEditorLayer.h"

#include <filesystem>

#include <ImGuizmo.h>
#include <imgui.h>

#include "commands/EntityCommands.h"
#include "engine/Application.h"
#include "engine/Log.h"
#include "engine/input/InputManager.h"

namespace mst {

MapEditorLayer::MapEditorLayer() : Layer("MapEditorLayer") {}

MapEditorLayer::~MapEditorLayer() = default;

void MapEditorLayer::OnAttach() {
    Layer::OnAttach();
    SE_LOG_INFO("MapEditorLayer::OnAttach");
    
    context_ = CreateScope<EditorContext>();
    context_->SetFileDialogManager(&fileDialogs_);
    
    viewportPanel_ = CreateScope<ViewportPanel>();
    statusBarPanel_ = CreateScope<StatusBarPanel>();
    
    // Connect panel visibility to menu bar
    PanelVisibility visibility;
    visibility.viewport = &viewportVisible_;
    visibility.hierarchy = &hierarchyVisible_;
    visibility.properties = &propertiesVisible_;
    visibility.statusBar = &statusBarVisible_;
    menuBar_.SetPanelVisibility(visibility);
    
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
    
    // Sync ECS world transforms to Filament renderables before rendering.
    // Without this, entities created via PrimitiveFactory would never appear.
    scene.OnRender();
    
    renderer.RenderFrame(camera, scene);
    renderer.RenderGrid(camera);
    renderer.RenderColliderDebug(camera, scene);
}

void MapEditorLayer::OnImGuiRender() {
    Layer::OnImGuiRender();
    ImGuizmo::BeginFrame();
    
    SetupDockspace();
    
    MenuBarActions menuActions = menuBar_.Render();
    ProcessMenuActions(menuActions);
    
    ImGui::End();  // End dockspace
    
    if (hierarchyVisible_) {
        hierarchyPanel_.Render(context_->GetScene(), context_->GetSelection());
        ProcessHierarchyActions();
    }
    
    if (propertiesVisible_) {
        propertiesPanel_.Render(context_->GetSelection(), context_->GetGizmo(), *context_);
    }
    
    if (viewportVisible_) {
        viewportPanel_->Render(*context_);
    }
    
    if (statusBarVisible_) {
        statusBarPanel_->SetGridVisible(viewportPanel_->GetRenderer().GetSettings().showGrid);
        statusBarPanel_->SetColliderDebugVisible(viewportPanel_->GetRenderer().GetSettings().showColliderDebug);
        statusBarPanel_->Render(*context_);
    }
    
    fileDialogs_.Render(*context_);
    
    // Material Editor Panel (separate window)
    materialEditorPanel_.Render(*context_);
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
    if (!viewportPanel_->IsHovered() && !viewportPanel_->IsFocused()) return;
    if (context_->GetGizmo().IsUsing()) return;
    
    auto& input = se::InputManager::Get();
    auto& camera = context_->GetCamera();
    
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
    
    // Right-click: rotate camera + WASD movement
    if (rightButton) {
        camera.OnMouseMove(dx, dy, false, false, true);
        
        // WASD movement while right-click is held
        if (input.IsKeyDown(GLFW_KEY_W)) {
            camera.MoveForward(ts);
        }
        if (input.IsKeyDown(GLFW_KEY_S)) {
            camera.MoveForward(-ts);
        }
        if (input.IsKeyDown(GLFW_KEY_A)) {
            camera.MoveRight(-ts);
        }
        if (input.IsKeyDown(GLFW_KEY_D)) {
            camera.MoveRight(ts);
        }
        if (input.IsKeyDown(GLFW_KEY_E) || input.IsKeyDown(GLFW_KEY_SPACE)) {
            camera.MoveUp(ts);
        }
        if (input.IsKeyDown(GLFW_KEY_Q) || input.IsKeyDown(GLFW_KEY_LEFT_CONTROL)) {
            camera.MoveUp(-ts);
        }
    }
    
    // Alt + left-click: orbit (legacy)
    if (altPressed && leftButton) {
        camera.OnMouseMove(dx, dy, false, false, true);
    }
    
    // Middle-click: pan
    if (middleButton) {
        camera.OnMouseMove(dx, dy, false, true, false);
    }
    
    // Mouse scroll: zoom (only when viewport is hovered)
    if (viewportPanel_->IsHovered()) {
        float scroll = ImGui::GetIO().MouseWheel;
        if (scroll != 0.0f) {
            camera.OnMouseScroll(scroll);
        }
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
                context_->GetDocument().Open(result.fullPath);
                context_->GetCommandSystem().Clear();
                
                // Load compiled materials from binary files and apply to entities
                // Assets base path is two directories up from the map file (maps/ folder)
                std::filesystem::path mapPath(result.fullPath);
                std::string assetsBase = mapPath.parent_path().parent_path().string();
                context_->LoadCompiledMaterialsFromMap(assetsBase);
                
                SE_LOG_INFO("Opened map with compiled materials: {}", result.fullPath);
            }
        });
    }
    
    if (actions.exportMap) {
        fileDialogs_.ShowSaveDialog([this](const FileDialogResult& result) {
            if (result.confirmed) {
                context_->GetDocument().SetMapName(result.filename);
                // Use SaveMapWithCompiledMaterials to compile and save material binaries
                context_->SaveMapWithCompiledMaterials(result.fullPath);
                SE_LOG_INFO("Saved map with compiled materials: {}", result.fullPath);
            }
        });
    }
    
    if (actions.exitApp) {
        se::Application::Get().Close();
    }
    
    if (actions.createCube) {
        context_->GetCommandSystem().Execute(
            std::make_unique<CreatePrimitiveCommand>(*context_, PrimitiveType::Cube));
    }
    if (actions.createSphere) {
        context_->GetCommandSystem().Execute(
            std::make_unique<CreatePrimitiveCommand>(*context_, PrimitiveType::Sphere));
    }
    if (actions.createCapsule) {
        context_->GetCommandSystem().Execute(
            std::make_unique<CreatePrimitiveCommand>(*context_, PrimitiveType::Capsule));
    }
    if (actions.createCylinder) {
        context_->GetCommandSystem().Execute(
            std::make_unique<CreatePrimitiveCommand>(*context_, PrimitiveType::Cylinder));
    }
    if (actions.createPlane) {
        context_->GetCommandSystem().Execute(
            std::make_unique<CreatePrimitiveCommand>(*context_, PrimitiveType::Plane));
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
        context_->GetCommandSystem().Execute(
            std::make_unique<DeleteEntitiesCommand>(*context_, entities));
    }
    
    if (actions.duplicateSelected && selection.HasSelection()) {
        auto entity = selection.GetPrimarySelection();
        context_->GetCommandSystem().Execute(
            std::make_unique<DuplicateEntityCommand>(*context_, entity));
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
    
    if (actions.openMaterialEditor) {
        materialEditorPanel_.ToggleVisible();
    }
}

void MapEditorLayer::ProcessHierarchyActions() {
    auto& selection = context_->GetSelection();
    
    if (hierarchyPanel_.WantsDelete() && selection.HasSelection()) {
        auto entities = selection.GetSelectedEntities();
        selection.ClearSelection();
        context_->GetCommandSystem().Execute(
            std::make_unique<DeleteEntitiesCommand>(*context_, entities));
    }
    
    if (hierarchyPanel_.WantsDuplicate() && selection.HasSelection()) {
        auto entity = selection.GetPrimarySelection();
        context_->GetCommandSystem().Execute(
            std::make_unique<DuplicateEntityCommand>(*context_, entity));
    }
    
    hierarchyPanel_.ClearActions();
}

}  // namespace mst
