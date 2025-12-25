#pragma once
/**
 * ViewportRenderer.h - Framebuffer rendering management.
 *
 * Handles the render-to-texture pipeline for the editor viewport,
 * including grid and collider debug visualization.
 */

#include <memory>

#include "Engine.h"
#include "EditorCamera.h"
#include "EditorFramebuffer.h"
#include "EditorGrid.h"
#include "ColliderDebugRenderer.h"
#include "engine/ecs/Scene.h"

namespace mst {

struct ViewportSettings {
    bool showGrid = true;
    bool showColliderDebug = false;
    se::Vector3 clearColor = {0.12f, 0.12f, 0.15f};
};

class ViewportRenderer {
public:
    ViewportRenderer(uint32_t width = 1280, uint32_t height = 720);
    
    void BeginFrame();
    void RenderScene(const EditorCamera& camera, se::Scene& scene);
    void RenderGrid(const EditorCamera& camera);
    void RenderColliderDebug(const EditorCamera& camera, se::Scene& scene);
    void EndFrame();
    
    void Resize(uint32_t width, uint32_t height);
    
    uint32_t GetColorAttachment() const;
    uint32_t GetWidth() const { return width_; }
    uint32_t GetHeight() const { return height_; }
    float GetAspectRatio() const;
    
    ViewportSettings& GetSettings() { return settings_; }

private:
    se::Scope<EditorFramebuffer> framebuffer_;
    se::Scope<EditorGrid> grid_;
    se::Scope<ColliderDebugRenderer> colliderDebug_;
    
    uint32_t width_;
    uint32_t height_;
    ViewportSettings settings_;
};

}  // namespace mst
