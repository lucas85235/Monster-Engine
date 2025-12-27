#pragma once

namespace se {

class Scene;

// System that automatically updates all AnimatorComponents and BoneAttachments.
// Called by Scene::OnUpdate() to tick animations every frame.
class AnimationSystem {
   public:
    static void Update(Scene& scene, float deltaTime);
    static void UpdateBoneAttachments(Scene& scene);

   private:
    AnimationSystem() = delete;
};

}  // namespace se
