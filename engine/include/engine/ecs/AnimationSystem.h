#pragma once

namespace se {

class Scene;

// System that automatically updates all AnimatorComponents in a scene.
// Called by Scene::OnUpdate() to tick animations every frame.
class AnimationSystem {
   public:
    static void Update(Scene& scene, float deltaTime);

   private:
    AnimationSystem() = delete;
};

}  // namespace se
