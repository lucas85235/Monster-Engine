
#pragma once

#include "engine/Camera.h"
#include "engine/renderer/RenderCommand.h"
#include "engine/renderer/SceneRenderer.h"
#include <memory>

namespace se {

// Forward declarations
class Window;

class Renderer {
  public:
    Renderer();
    ~Renderer();

    // Initialize renderer subsystems
    void Init();
    void Shutdown();

    // Begin/End frame
    void BeginFrame();
    void EndFrame();

    // Clear screen
    void Clear();
    void SetClearColor(float r, float g, float b, float a = 1.0f);

    // Scene rendering
    void BeginScene(const Camera& camera, float aspectRatio);
    void EndScene();

    // Get renderer stats
    RenderStats GetStats() const {
        return SceneRenderer::GetStats();
    }
    void ResetStats() {
        SceneRenderer::ResetStats();
    }

    // Disable copy/move
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

  private:
    bool initialized_ = false;
};

} // namespace se