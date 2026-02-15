#pragma once

namespace se {

class Scene;

/**
 * Synchronizes ECS scene data into Filament runtime objects.
 */
class FilamentRenderBridge {
   public:
    static void SyncScene(Scene& scene);
};

}  // namespace se

