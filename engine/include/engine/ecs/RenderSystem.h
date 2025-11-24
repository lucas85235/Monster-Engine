
#pragma once

#include "engine/Camera.h"
#include <glm.hpp>

namespace se {

// Forward declarations
class Scene;

class RenderSystem {
  public:
    static void Init();
    static void Shutdown();

    // Render all entities with MeshRenderComponent in the scene
    static void Render(Scene& scene, const Camera& camera, float aspectRatio);

  private:
    RenderSystem() = delete;
    static bool initialized_;
};

} // namespace se