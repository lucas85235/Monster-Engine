#include "engine/renderer/FilamentContext.h"

#include <filament/Engine.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/View.h>
#include <filament/Camera.h>
#include <filament/SwapChain.h>
#include <filament/Viewport.h>

#include <utils/EntityManager.h>

#include <spdlog/spdlog.h>

// Platform-specific native window extraction
#if defined(__APPLE__)
extern "C" void* GetCocoaNativeWindow(void* glfwWindow);
#elif defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#endif

namespace se {

namespace {

/**
 * Convert our Backend enum to Filament's backend enum.
 */
filament::backend::Backend ToFilamentBackend(FilamentContext::Backend backend) {
    switch (backend) {
        case FilamentContext::Backend::OpenGL:
            return filament::backend::Backend::OPENGL;
        case FilamentContext::Backend::Vulkan:
            return filament::backend::Backend::VULKAN;
        case FilamentContext::Backend::Metal:
            return filament::backend::Backend::METAL;
        case FilamentContext::Backend::Default:
        default:
            return filament::backend::Backend::DEFAULT;
    }
}

} // anonymous namespace

FilamentContext::~FilamentContext() {
    Shutdown();
}

void FilamentContext::Init(GLFWwindow* window, uint32_t width, uint32_t height,
                           Backend backend) {
    if (engine_) {
        spdlog::warn("FilamentContext::Init called but engine already exists. Ignoring.");
        return;
    }

    active_backend_ = backend;

    // Create the Filament engine with the specified backend
    engine_ = filament::Engine::create(ToFilamentBackend(backend));
    if (!engine_) {
        spdlog::critical("Failed to create Filament Engine!");
        return;
    }

    spdlog::info("Filament Engine created successfully.");

    // Create SwapChain from the native window
    void* nativeWindow = GetNativeWindowHandle(window);
    if (!nativeWindow) {
        spdlog::critical("Failed to get native window handle!");
        Shutdown();
        return;
    }

    swap_chain_ = engine_->createSwapChain(nativeWindow);
    if (!swap_chain_) {
        spdlog::critical("Failed to create Filament SwapChain!");
        Shutdown();
        return;
    }

    // Create the renderer
    renderer_ = engine_->createRenderer();

    // Create the scene
    scene_ = engine_->createScene();

    // Create the view and associate it with the scene
    view_ = engine_->createView();
    view_->setScene(scene_);
    view_->setViewport({0, 0, width, height});

    // Enable post-processing features
    view_->setPostProcessingEnabled(true);

    // Create camera
    auto& entityManager = utils::EntityManager::get();
    auto cameraEntity = entityManager.create();
    camera_ = engine_->createCamera(cameraEntity);
    view_->setCamera(camera_);

    // Store the entity for cleanup (use heap allocation to avoid header dependency)
    camera_entity_ = new utils::Entity(cameraEntity);

    spdlog::info("FilamentContext initialized: {}x{}, Backend={}",
                 width, height,
                 backend == Backend::Metal   ? "Metal"   :
                 backend == Backend::Vulkan  ? "Vulkan"  :
                 backend == Backend::OpenGL  ? "OpenGL"  : "Default");
}

void FilamentContext::Shutdown() {
    if (!engine_) return;

    if (camera_entity_) {
        engine_->destroyCameraComponent(*camera_entity_);
        utils::EntityManager::get().destroy(*camera_entity_);
        delete camera_entity_;
        camera_entity_ = nullptr;
        camera_ = nullptr;
    }

    if (view_) {
        engine_->destroy(view_);
        view_ = nullptr;
    }

    if (scene_) {
        engine_->destroy(scene_);
        scene_ = nullptr;
    }

    if (renderer_) {
        engine_->destroy(renderer_);
        renderer_ = nullptr;
    }

    if (swap_chain_) {
        engine_->destroy(swap_chain_);
        swap_chain_ = nullptr;
    }

    filament::Engine::destroy(&engine_);
    engine_ = nullptr;

    spdlog::info("FilamentContext shut down.");
}

void FilamentContext::OnResize(uint32_t width, uint32_t height) {
    if (!view_) return;
    view_->setViewport({0, 0, width, height});
}

void* FilamentContext::GetNativeWindowHandle(GLFWwindow* window) const {
#if defined(__APPLE__)
    // Implemented in FilamentNativeWindow.mm (Objective-C++)
    return GetCocoaNativeWindow(static_cast<void*>(window));
#elif defined(_WIN32)
    return static_cast<void*>(glfwGetWin32Window(window));
#elif defined(__linux__)
    return reinterpret_cast<void*>(glfwGetX11Window(window));
#else
    spdlog::error("Unsupported platform for Filament native window!");
    return nullptr;
#endif
}

} // namespace se
