#include "engine/Renderer.h"

#include "engine/Log.h"
#include "engine/core/ServiceLocator.h"
#include "engine/ecs/RenderSystem.h"
#include "engine/resources/MaterialManager.h"
#include "engine/resources/MeshManager.h"
#include "engine/resources/ModelManager.h"
#include "engine/resources/TextureManager.h"

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
    sceneRenderer_.Init();

    // Register SceneRenderer with ServiceLocator
    ServiceLocator::Get().ProvideSceneRenderer(&sceneRenderer_);

    // Initialize resource managers
    TextureManager::Init();
    MeshManager::Init();
    MaterialManager::Init();
    ModelManager::Init();

    // Initialize render system
    RenderSystem::Init();

    initialized_ = true;
    SE_LOG_INFO("Renderer initialized successfully");
}

void Renderer::Shutdown() {
    if (!initialized_) return;

    SE_LOG_INFO("Shutting down Renderer");

    RenderSystem::Shutdown();
    ModelManager::Shutdown();
    MaterialManager::Shutdown();
    MeshManager::Shutdown();
    TextureManager::Shutdown();
    sceneRenderer_.Shutdown();

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
    sceneRenderer_.BeginScene(camera, projection);
}

void Renderer::EndScene() {
    sceneRenderer_.EndScene();
}

}  // namespace se