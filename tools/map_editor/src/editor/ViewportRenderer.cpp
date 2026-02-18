#include "ViewportRenderer.h"

#include <filament/Camera.h>
#include <filament/Renderer.h>
#include <filament/View.h>
#include <math/mat4.h>

#include "engine/Application.h"
#include "engine/renderer/FilamentRenderer.h"
#include "engine/renderer/LightSystem.h"
#include "engine/Log.h"

using namespace se;

namespace mst {

ViewportRenderer::ViewportRenderer(uint32_t width, uint32_t height)
    : width_(width), height_(height) {
    framebuffer_ = se::CreateScope<EditorFramebuffer>(width, height);
    grid_ = se::CreateScope<EditorGrid>();
    colliderDebug_ = se::CreateScope<ColliderDebugRenderer>();

    // Create default directional light for the editor viewport.
    // Without a light, Filament's lit materials render as solid black.
    auto& lightSystem = se::Application::Get().GetLightSystem();
    lightSystem.SetDirectionalLight(
        0.3f, -1.0f, -0.5f,   // direction (top-left, slightly forward)
        1.0f, 0.98f, 0.95f,   // warm white color
        100000.0f,             // intensity (lux — standard for SUN type)
        true                   // cast shadows
    );

    // Initialize the editor grid (Filament renderable added to the scene).
    if (grid_) {
        grid_->Init();
    }

    // Set the Filament scene on the offscreen view
    auto* filamentScene = se::Application::Get().GetFilamentContext().GetScene();
    if (framebuffer_ && filamentScene) {
        framebuffer_->SetScene(filamentScene);
    }

    // Register the offscreen view with the engine's renderer so it is
    // rendered BEFORE the main view each frame (texture ready for ImGui).
    if (framebuffer_ && framebuffer_->GetView()) {
        se::Application::Get().GetFilamentRenderer().RegisterOffscreenView(
            framebuffer_->GetView());
    }
    
    SE_LOG_INFO("ViewportRenderer: Initialized {}x{}", width, height);
}

ViewportRenderer::~ViewportRenderer() {
    // Unregister the offscreen view when destroyed
    if (framebuffer_ && framebuffer_->GetView()) {
        se::Application::Get().GetFilamentRenderer().UnregisterOffscreenView(
            framebuffer_->GetView());
    }
}

void ViewportRenderer::RenderFrame(const EditorCamera& camera, se::Scene& scene) {
    if (!framebuffer_ || !framebuffer_->GetView()) return;

    // Sync engine Camera → filament::Camera each frame.
    // The actual render() call is handled by FilamentRenderer::EndFrame()
    // via the registered offscreen view.
    auto* filCamera = framebuffer_->GetCamera();
    if (filCamera) {
        const auto& engineCam = camera.GetCamera();
        
        // Set camera position and orientation using lookAt
        auto pos   = engineCam.GetPosition();
        auto front = engineCam.GetFront();
        auto up    = engineCam.GetUp();
        
        filCamera->lookAt(
            filament::math::float3{pos.x, pos.y, pos.z},
            filament::math::float3{pos.x + front.x, pos.y + front.y, pos.z + front.z},
            filament::math::float3{up.x, up.y, up.z}
        );
        
        // Set projection (perspective, matching engine camera FOV)
        float aspectRatio = GetAspectRatio();
        float fov = engineCam.GetZoom();  // FOV in degrees
        filCamera->setProjection(fov, aspectRatio, 0.1f, 1000.0f,
                                 filament::Camera::Fov::VERTICAL);
    }
}

void ViewportRenderer::RenderGrid(const EditorCamera& /*camera*/) {
    // Grid is now a Filament scene renderable — rendered automatically
    // by FilamentRenderer::EndFrame() via the offscreen view.
    // Visibility is controlled by showing/hiding the renderable.
    (void)settings_.showGrid;
}

void ViewportRenderer::RenderColliderDebug(const EditorCamera& /*camera*/, se::Scene& /*scene*/) {
    // ColliderDebugRenderer is still stubbed (no-op).
    // Future: will use Filament wireframe renderables.
}

void ViewportRenderer::Resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;
    if (width == width_ && height == height_) return;

    // Unregister old view before destroying
    if (framebuffer_ && framebuffer_->GetView()) {
        se::Application::Get().GetFilamentRenderer().UnregisterOffscreenView(
            framebuffer_->GetView());
    }
    
    width_ = width;
    height_ = height;
    
    if (framebuffer_) {
        framebuffer_->Resize(width, height);
        // Re-set the scene and re-register the new view after resize
        auto* filamentScene = se::Application::Get().GetFilamentContext().GetScene();
        if (filamentScene) {
            framebuffer_->SetScene(filamentScene);
        }
        if (framebuffer_->GetView()) {
            se::Application::Get().GetFilamentRenderer().RegisterOffscreenView(
                framebuffer_->GetView());
        }
    }
}

uintptr_t ViewportRenderer::GetColorAttachmentAsImTextureID() const {
    return framebuffer_ ? framebuffer_->GetColorAttachmentAsImTextureID() : 0;
}

float ViewportRenderer::GetAspectRatio() const {
    if (height_ == 0) return 1.0f;
    return static_cast<float>(width_) / static_cast<float>(height_);
}

}  // namespace mst
