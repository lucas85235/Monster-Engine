#pragma once

#include <memory>

#include "engine/renderer/Camera.h"
#include "engine/renderer/RenderCommand.h"
#include "engine/renderer/SceneRenderer.h"

namespace se {

class Window;

class Renderer {
   public:
    Renderer();
    ~Renderer();

    void Init();
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void Clear();
    void SetClearColor(float r, float g, float b, float a = 1.0f);

    void BeginScene(const Camera& camera, float aspectRatio);
    void EndScene();

    // Access to SceneRenderer instance
    SceneRenderer& GetSceneRenderer() { return sceneRenderer_; }

    RenderStats GetStats() const { return sceneRenderer_.GetStats(); }
    void ResetStats() { sceneRenderer_.ResetStats(); }

    // Disable copy/move
    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

   private:
    SceneRenderer sceneRenderer_;
    bool initialized_ = false;
};

}  // namespace se