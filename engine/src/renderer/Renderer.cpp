#include "engine/renderer/Renderer.h"

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/core/ServiceLocator.h"
#include "engine/ecs/RenderSystem.h"
#include "engine/renderer/GraphicsContext.h"
#include "engine/resources/MaterialManager.h"
#include "engine/resources/MeshManager.h"

namespace se {

// Helper to get device
static RHI::IDevice* GetDevice() {
    auto& app     = Application::Get();
    auto& window  = app.GetWindow();
    auto* context = window.GetContext();
    return context ? context->GetDevice() : nullptr;
}

Renderer::Renderer() {}

Renderer::~Renderer() {
    Shutdown();
}

void Renderer::Init() {
    if (initialized_) {
        SE_LOG_WARN("Renderer already initialized");
        return;
    }

    SE_LOG_INFO("Initializing Renderer");

    // Initialize low-level rendering systems
    // RenderCommand::Init() removed as RHI init is handled by Window/GraphicsContext
    sceneRenderer_.Init();

    // Register SceneRenderer with ServiceLocator
    ServiceLocator::Get().ProvideSceneRenderer(&sceneRenderer_);

    // Initialize resource managers
    MeshManager::Init();
    MaterialManager::Init();

    // Initialize render system
    RenderSystem::Init();

    initialized_ = true;
    SE_LOG_INFO("Renderer initialized successfully");
}

void Renderer::Shutdown() {
    if (!initialized_) return;

    SE_LOG_INFO("Shutting down Renderer");

    RenderSystem::Shutdown();
    MaterialManager::Shutdown();
    MeshManager::Shutdown();
    sceneRenderer_.Shutdown();

    initialized_ = false;
}

void Renderer::BeginFrame() {
    // Start the frame on the RHI device (begins command buffer, render pass, etc.)
    if (auto* device = GetDevice()) { 
        device->BeginFrame(); 
    }
}

void Renderer::EndFrame() {
    // End the frame on the RHI device (ends render pass, submits command buffer, presents)
    if (auto* device = GetDevice()) { 
        device->EndFrame(); 
    }
}

void Renderer::Clear() {
    if (auto* device = GetDevice()) { device->Clear(true, true, false); }
}

void Renderer::SetClearColor(float r, float g, float b, float a) {
    if (auto* device = GetDevice()) { device->SetClearColor({r, g, b, a}); }
}

void Renderer::BeginScene(const Camera& camera, float aspectRatio) {
    Matrix4 projection = camera.getProjectionMatrix(aspectRatio);
    sceneRenderer_.BeginScene(camera, projection);
}

void Renderer::EndScene() {
    sceneRenderer_.EndScene();
}

}  // namespace se