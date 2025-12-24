#include "MapEditorLayer.h"

#include <glad/glad.h>
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
    
    // Create framebuffer for viewport
    framebuffer_ = CreateScope<EditorFramebuffer>(viewportWidth_, viewportHeight_);
    
    // Create grid renderer
    editorGrid_ = CreateScope<EditorGrid>();
    
    // Create collider debug renderer
    colliderDebug_ = CreateScope<ColliderDebugRenderer>();
    
    scene_ = CreateScope<se::Scene>("Editor Scene", se::SceneSettings{.EnablePhysics = false});
    se::Application::Get().SetActiveScene(scene_.get());
    
    // Add directional light for scene illumination
    auto sunEntity = scene_->CreateEntity("Editor Light");
    auto& sunTransform = sunEntity.GetComponent<se::TransformComponent>();
    sunTransform.SetPosition({0.0f, 10.0f, 10.0f});
    sunTransform.SetRotation({-45.0f, 0.0f, 0.0f});
    auto& sunLight = sunEntity.AddComponent<se::DirectionalLightComponent>();
    sunLight.Color = {1.0f, 0.98f, 0.9f};
    sunLight.Intensity = 1.5f;
    sunLight.CastShadows = true;
    SE_LOG_INFO("Editor light created");
    
    editorCamera_.FocusOnPoint({0.0f, 0.0f, 0.0f});
}

void MapEditorLayer::OnDetach() {
    SE_LOG_INFO("MapEditorLayer::OnDetach");
    framebuffer_.reset();
    se::Application::Get().SetActiveScene(nullptr);
    scene_.reset();
    Layer::OnDetach();
}

void MapEditorLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);
    auto& input = se::InputManager::Get();
    
    // Calculate mouse delta
    float mouseX = input.GetMousePosition().x;
    float mouseY = input.GetMousePosition().y;
    float dx = mouseX - lastMouseX_;
    float dy = mouseY - lastMouseY_;
    lastMouseX_ = mouseX;
    lastMouseY_ = mouseY;
    
    // Camera controls only when viewport is hovered and not using gizmo
    if (viewportHovered_ && !gizmo_.IsUsing()) {
        bool altPressed = input.IsKeyDown(GLFW_KEY_LEFT_ALT) || input.IsKeyDown(GLFW_KEY_RIGHT_ALT);
        bool leftButton = input.IsMouseButtonDown(0);
        bool middleButton = input.IsMouseButtonDown(2);
        bool rightButton = input.IsMouseButtonDown(1);
        
        // Alt+Left = Orbit, Middle = Pan, Alt+Right or Right = Orbit
        bool orbiting = (altPressed && leftButton) || rightButton;
        bool panning = middleButton || (altPressed && middleButton);
        
        if (orbiting) {
            editorCamera_.OnMouseMove(dx, dy, false, false, true);
        } else if (panning) {
            editorCamera_.OnMouseMove(dx, dy, false, true, false);
        }
        
        // Scroll to zoom
        float scroll = ImGui::GetIO().MouseWheel;
        if (scroll != 0.0f) {
            editorCamera_.OnMouseScroll(scroll);
        }
    }
    
    editorCamera_.Update(ts);
    scene_->OnUpdate(ts);
}

void MapEditorLayer::OnRender() {
    Layer::OnRender();
    
    // Render scene to framebuffer
    if (framebuffer_) {
        framebuffer_->Bind();
        
        // Dark editor background
        glClearColor(0.12f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Render scene with viewport aspect ratio
        float aspectRatio = static_cast<float>(viewportWidth_) / static_cast<float>(viewportHeight_);
        
        // Debug log camera position occasionally
        static int frameCount = 0;
        if (frameCount % 300 == 0) {
            auto camPos = editorCamera_.GetCamera().GetPosition();
            SE_LOG_INFO("Render: Camera at ({},{},{}), FBO {}x{}, aspectRatio={}", 
                camPos.x, camPos.y, camPos.z, viewportWidth_, viewportHeight_, aspectRatio);
        }
        frameCount++;
        
        // Render grid first (so objects render on top)
        if (showGrid_) {
            Matrix4 view = editorCamera_.GetCamera().getViewMatrix();
            Matrix4 projection = editorCamera_.GetCamera().getProjectionMatrix(aspectRatio);
            RenderGrid(view, projection);
        }
        
        scene_->OnRender(editorCamera_.GetCamera(), aspectRatio);
        
        // Render collider debug wireframes
        if (showColliderDebug_ && colliderDebug_) {
            Matrix4 view = editorCamera_.GetCamera().getViewMatrix();
            Matrix4 projection = editorCamera_.GetCamera().getProjectionMatrix(aspectRatio);
            colliderDebug_->Render(view, projection, *scene_);
        }
        
        framebuffer_->Unbind();
    }
    
    // Restore main window viewport
    auto& window = se::Application::Get().GetWindow();
    glViewport(0, 0, window.GetWidth(), window.GetHeight());
}

void MapEditorLayer::OnImGuiRender() {
    Layer::OnImGuiRender();
    ImGuizmo::BeginFrame();
    
    // Standard dockspace setup
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
    if (showOpenDialog_) ShowOpenDialog();
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
    if (actions.openMap) showOpenDialog_ = true;
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
    if (actions.toggleColliderDebug) {
        showColliderDebug_ = !showColliderDebug_;
        SE_LOG_INFO("Collider Debug: {}", showColliderDebug_ ? "ON" : "OFF");
    }
    if (actions.resetCamera) {
        editorCamera_.FocusOnPoint({0.0f, 0.0f, 0.0f});
    }
}

void MapEditorLayer::ProcessHierarchyActions() {
    if (hierarchy_.WantsDelete()) DeleteSelected();
    if (hierarchy_.WantsDuplicate()) DuplicateSelected();
    hierarchy_.ClearActions();
}

void MapEditorLayer::ProcessKeyboardShortcuts() {
    auto& input = se::InputManager::Get();
    
    // Gizmo mode switching (W/E/R) - only when viewport is hovered/focused
    if (viewportHovered_ || viewportFocused_) {
        if (input.IsKeyDown(GLFW_KEY_W) && !input.IsKeyDown(GLFW_KEY_LEFT_CONTROL)) {
            gizmo_.SetOperation(GizmoController::Operation::Translate);
        }
        if (input.IsKeyDown(GLFW_KEY_E) && !input.IsKeyDown(GLFW_KEY_LEFT_CONTROL)) {
            gizmo_.SetOperation(GizmoController::Operation::Rotate);
        }
        if (input.IsKeyDown(GLFW_KEY_R) && !input.IsKeyDown(GLFW_KEY_LEFT_CONTROL)) {
            gizmo_.SetOperation(GizmoController::Operation::Scale);
        }
        if (input.IsKeyDown(GLFW_KEY_Q) && !input.IsKeyDown(GLFW_KEY_LEFT_CONTROL)) {
            gizmo_.ToggleSpace();
        }
    }
    
    // Global shortcuts (work regardless of focus)
    
    // Delete - one-shot
    bool deletePressed = input.IsKeyDown(GLFW_KEY_DELETE);
    if (deletePressed && !wasKeyDeletePressed_) {
        DeleteSelected();
    }
    wasKeyDeletePressed_ = deletePressed;
    
    // Duplicate (Ctrl+D)
    static bool wasCtrlDPressed = false;
    bool ctrlDPressed = input.IsKeyDown(GLFW_KEY_LEFT_CONTROL) && input.IsKeyDown(GLFW_KEY_D);
    if (ctrlDPressed && !wasCtrlDPressed) {
        DuplicateSelected();
    }
    wasCtrlDPressed = ctrlDPressed;
    
    // Focus on selection (F) - one-shot
    bool fPressed = input.IsKeyDown(GLFW_KEY_F);
    if (fPressed && !wasKeyFPressed_ && selection_.HasSelection()) {
        auto entity = selection_.GetPrimarySelection();
        if (entity.IsValid() && entity.HasComponent<se::TransformComponent>()) {
            auto& transform = entity.GetComponent<se::TransformComponent>();
            editorCamera_.FocusOnPoint(transform.Position);
            SE_LOG_INFO("Camera focused on entity at ({}, {}, {})", 
                transform.Position.x, transform.Position.y, transform.Position.z);
        }
    }
    wasKeyFPressed_ = fPressed;
    
    // Grid toggle (G) - one-shot
    bool gPressed = input.IsKeyDown(GLFW_KEY_G);
    if (gPressed && !wasKeyGPressed_) {
        showGrid_ = !showGrid_;
        SE_LOG_INFO("Grid: {}", showGrid_ ? "ON" : "OFF");
    }
    wasKeyGPressed_ = gPressed;
    
    // Collider debug toggle (C) - one-shot
    bool cPressed = input.IsKeyDown(GLFW_KEY_C);
    if (cPressed && !wasKeyCPressed_) {
        showColliderDebug_ = !showColliderDebug_;
        SE_LOG_INFO("Collider Debug: {}", showColliderDebug_ ? "ON" : "OFF");
    }
    wasKeyCPressed_ = cPressed;
    
    // Export (Ctrl+E)
    if (input.IsKeyDown(GLFW_KEY_LEFT_CONTROL) && input.IsKeyDown(GLFW_KEY_E)) {
        showExportDialog_ = true;
    }
    
    // Escape to deselect
    if (input.IsKeyDown(GLFW_KEY_ESCAPE)) {
        selection_.ClearSelection();
    }
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

void MapEditorLayer::ShowOpenDialog() {
    ImGui::OpenPopup("Open Map");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Open Map", &showOpenDialog_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Open a .mstmap file");
        ImGui::Separator();
        ImGui::InputText("File Name", openFileName_, sizeof(openFileName_));
        ImGui::Text("(Enter filename without extension, file must be in current directory)");
        ImGui::Separator();
        if (ImGui::Button("Open", ImVec2(120, 0))) {
            LoadMap(std::string(openFileName_) + ".mstmap");
            showOpenDialog_ = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) showOpenDialog_ = false;
        ImGui::EndPopup();
    }
}

void MapEditorLayer::LoadMap(const std::string& filename) {
    MapData loadedData;
    std::filesystem::path path = filename;
    
    if (!MapSerializer::Import(loadedData, path)) {
        SE_LOG_ERROR("Failed to load map from '{}'", filename);
        return;
    }
    
    // Clear current scene
    selection_.ClearSelection();
    scene_->Clear();
    
    // Recreate the light
    auto sunEntity = scene_->CreateEntity("Editor Light");
    auto& sunTransform = sunEntity.GetComponent<se::TransformComponent>();
    sunTransform.SetPosition({0.0f, 10.0f, 10.0f});
    sunTransform.SetRotation({-45.0f, 0.0f, 0.0f});
    auto& sunLight = sunEntity.AddComponent<se::DirectionalLightComponent>();
    sunLight.Color = {1.0f, 0.98f, 0.9f};
    sunLight.Intensity = 1.5f;
    sunLight.CastShadows = true;
    
    // Create entities from loaded data
    for (const auto& entityData : loadedData.entities) {
        // Create primitive with the saved name
        se::Entity entity = PrimitiveFactory::CreatePrimitive(*scene_, entityData.primitiveType, entityData.name);
        
        // Apply transform
        auto& transform = entity.GetComponent<se::TransformComponent>();
        transform.SetPosition(entityData.position);
        transform.SetRotation(entityData.rotation);
        transform.SetScale(entityData.scale);
        
        // Apply color if has mesh render
        if (entity.HasComponent<se::MeshRenderComponent>()) {
            entity.GetComponent<se::MeshRenderComponent>().Color = entityData.color;
        }
        
        // Update editor metadata
        if (entity.HasComponent<PrimitiveFactory::EditorMetadata>()) {
            auto& metadata = entity.GetComponent<PrimitiveFactory::EditorMetadata>();
            metadata.hasCollision = entityData.hasCollision;
            metadata.colliderType = entityData.colliderType;
            metadata.colliderSize = entityData.colliderSize;
            metadata.colliderRadius = entityData.colliderRadius;
            metadata.colliderHeight = entityData.colliderHeight;
        }
    }
    
    currentMap_ = std::move(loadedData);
    strncpy(exportFileName_, currentMap_.mapName.c_str(), sizeof(exportFileName_) - 1);
    
    SE_LOG_INFO("Map '{}' loaded successfully with {} entities", 
                currentMap_.mapName, currentMap_.entities.size());
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

void MapEditorLayer::RenderGrid(const Matrix4& view, const Matrix4& projection) {
    if (editorGrid_) {
        editorGrid_->Render(view, projection);
    }
}

void MapEditorLayer::RenderViewport() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");
    
    // Track viewport state for camera controls
    viewportHovered_ = ImGui::IsWindowHovered();
    viewportFocused_ = ImGui::IsWindowFocused();
    
    // Get viewport dimensions
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    
    // Resize framebuffer if needed
    if (viewportSize.x > 0 && viewportSize.y > 0) {
        uint32_t newWidth = static_cast<uint32_t>(viewportSize.x);
        uint32_t newHeight = static_cast<uint32_t>(viewportSize.y);
        
        if (newWidth != viewportWidth_ || newHeight != viewportHeight_) {
            viewportWidth_ = newWidth;
            viewportHeight_ = newHeight;
            if (framebuffer_) {
                framebuffer_->Resize(viewportWidth_, viewportHeight_);
            }
        }
        
        // Display framebuffer texture (flip UV vertically for OpenGL)
        if (framebuffer_) {
            uint64_t textureId = framebuffer_->GetColorAttachment();
            ImGui::Image(reinterpret_cast<void*>(textureId), viewportSize, 
                         ImVec2(0, 1), ImVec2(1, 0));
        }
        
        // Save viewport position for picking
        viewportPos_ = ImGui::GetItemRectMin();
        viewportSize_ = viewportSize;
        
        // Setup ImGuizmo for this viewport (overlay on top of image)
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(viewportPos_.x, viewportPos_.y, viewportSize_.x, viewportSize_.y);
        
        float aspectRatio = viewportSize.x / viewportSize.y;
        
        // Grid is rendered in OnRender with proper depth testing
        // Render gizmo for selected entity
        if (selection_.HasSelection()) {
            auto entity = selection_.GetPrimarySelection();
            if (entity.IsValid() && entity.HasComponent<se::TransformComponent>()) {
                auto& transform = entity.GetComponent<se::TransformComponent>();
                gizmo_.Manipulate(editorCamera_.GetCamera(), aspectRatio, transform);
            }
        }
        
        // Process mouse clicking for object selection
        ProcessMousePicking();
    }
    
    ImGui::End();
    ImGui::PopStyleVar();
}

void MapEditorLayer::RenderStatusBar() {
    ImGui::Begin("Status Bar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("Entities: %zu", scene_->GetEntityCount());
    ImGui::SameLine(); ImGui::Text(" | ");
    ImGui::SameLine(); ImGui::Text("Selected: %zu", selection_.GetSelectedEntities().size());
    ImGui::SameLine(); ImGui::Text(" | ");
    ImGui::SameLine(); ImGui::Text("Grid: %s", showGrid_ ? "ON" : "OFF");
    ImGui::SameLine(); ImGui::Text(" | ");
    ImGui::SameLine(); ImGui::Text("Colliders: %s", showColliderDebug_ ? "ON" : "OFF");
    ImGui::SameLine(); ImGui::Text(" | ");
    ImGui::SameLine(); ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
    ImGui::End();
}

void MapEditorLayer::ProcessMousePicking() {
    // Only process when viewport is hovered and left mouse is clicked
    if (!viewportHovered_) return;
    
    bool isLeftMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    bool leftMouseJustPressed = isLeftMouseDown && !wasMouseLeftPressed_;
    wasMouseLeftPressed_ = isLeftMouseDown;
    
    if (!leftMouseJustPressed) return;
    
    // Don't pick if gizmo is being used
    if (ImGuizmo::IsOver()) return;
    
    // Get mouse position relative to viewport
    ImVec2 mousePos = ImGui::GetMousePos();
    float relX = mousePos.x - viewportPos_.x;
    float relY = mousePos.y - viewportPos_.y;
    
    // Check if mouse is inside viewport
    if (relX < 0 || relY < 0 || relX > viewportSize_.x || relY > viewportSize_.y) return;
    
    float aspectRatio = viewportSize_.x / viewportSize_.y;
    
    // Calculate ray direction from camera through mouse position
    Vector3 rayDir = ScreenToWorldRay(relX, relY, aspectRatio);
    Vector3 rayOrigin = editorCamera_.GetCamera().GetPosition();
    
    // Pick the entity
    se::Entity pickedEntity = PickEntity(rayOrigin, rayDir);
    
    if (pickedEntity.IsValid()) {
        selection_.ClearSelection();
        selection_.Select(pickedEntity);
        SE_LOG_INFO("Picked entity: {}", pickedEntity.GetComponent<se::NameComponent>().Name);
    } else {
        // Click on empty space clears selection
        selection_.ClearSelection();
    }
}

Vector3 MapEditorLayer::ScreenToWorldRay(float mouseX, float mouseY, float aspectRatio) {
    // Convert screen coordinates to normalized device coordinates (-1 to 1)
    float ndcX = (2.0f * mouseX / viewportSize_.x) - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY / viewportSize_.y);  // Flip Y for OpenGL
    
    // Get camera matrices
    Matrix4 projection = editorCamera_.GetCamera().getProjectionMatrix(aspectRatio);
    Matrix4 view = editorCamera_.GetCamera().getViewMatrix();
    
    // Inverse view-projection matrix
    Matrix4 invVP = glm::inverse(projection * view);
    
    // Near and far points in clip space
    Vector4 nearPoint = invVP * Vector4(ndcX, ndcY, -1.0f, 1.0f);
    Vector4 farPoint = invVP * Vector4(ndcX, ndcY, 1.0f, 1.0f);
    
    // Perspective divide
    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;
    
    // Ray direction
    Vector3 rayDir = glm::normalize(Vector3(farPoint) - Vector3(nearPoint));
    
    return rayDir;
}

se::Entity MapEditorLayer::PickEntity(const Vector3& rayOrigin, const Vector3& rayDir) {
    se::Entity closestEntity;
    float closestT = std::numeric_limits<float>::max();
    
    auto view = scene_->GetAllEntitiesWith<se::NameComponent, se::TransformComponent>();
    
    for (auto entityHandle : view) {
        se::Entity entity(entityHandle, scene_.get());
        
        // Skip editor light
        auto& name = entity.GetComponent<se::NameComponent>().Name;
        if (name == "Editor Light") continue;
        
        auto& transform = entity.GetComponent<se::TransformComponent>();
        Vector3 pos = transform.Position;
        Vector3 scale = transform.Scale;
        
        // Simple AABB based on scale (approximate bounding box)
        Vector3 halfSize = scale * 0.5f;
        Vector3 boxMin = pos - halfSize;
        Vector3 boxMax = pos + halfSize;
        
        float t = 0.0f;
        if (RayIntersectsAABB(rayOrigin, rayDir, boxMin, boxMax, t)) {
            if (t < closestT && t > 0.0f) {
                closestT = t;
                closestEntity = entity;
            }
        }
    }
    
    return closestEntity;
}

bool MapEditorLayer::RayIntersectsAABB(const Vector3& rayOrigin, const Vector3& rayDir,
                                        const Vector3& boxMin, const Vector3& boxMax, float& t) {
    float tmin = 0.0f;
    float tmax = std::numeric_limits<float>::max();
    
    for (int i = 0; i < 3; ++i) {
        if (std::abs(rayDir[i]) < 1e-8f) {
            // Ray is parallel to slab
            if (rayOrigin[i] < boxMin[i] || rayOrigin[i] > boxMax[i]) {
                return false;
            }
        } else {
            float invD = 1.0f / rayDir[i];
            float t1 = (boxMin[i] - rayOrigin[i]) * invD;
            float t2 = (boxMax[i] - rayOrigin[i]) * invD;
            
            if (t1 > t2) std::swap(t1, t2);
            
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            
            if (tmin > tmax) {
                return false;
            }
        }
    }
    
    t = tmin;
    return true;
}

}  // namespace mst

