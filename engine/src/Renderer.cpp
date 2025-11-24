#include "engine/Renderer.h"

#include "engine/Log.h"
#include "engine/ecs/RenderSystem.h"
#include "engine/resources/MaterialManager.h"
#include "engine/resources/MeshManager.h"

namespace se {

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
    RenderCommand::Init();
    SceneRenderer::Init();

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
    SceneRenderer::Shutdown();

    initialized_ = false;
}

void Renderer::BeginFrame() {
    // Any per-frame setup can go here
}

void Renderer::EndFrame() {
    // Any per-frame cleanup can go here
}

void Renderer::Clear() {
    RenderCommand::Clear();
}

void Renderer::SetClearColor(float r, float g, float b, float a) {
    RenderCommand::SetClearColor({r, g, b, a});
}

void Renderer::BeginScene(const Camera& camera, float aspectRatio) {
    Matrix4 projection = camera.getProjectionMatrix(aspectRatio);
    SceneRenderer::BeginScene(camera, projection);
}

void Renderer::EndScene() {
    SceneRenderer::EndScene();
}

}  // namespace se