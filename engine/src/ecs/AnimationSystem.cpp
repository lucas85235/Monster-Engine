#include "engine/ecs/AnimationSystem.h"

#include "engine/Log.h"
#include "engine/ecs/AnimatorComponent.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/ecs/SkinnedModelComponent.h"

namespace se {

void AnimationSystem::Update(Scene& scene, float deltaTime) {
    auto& registry = scene.GetRegistry();
    
    auto view = registry.view<AnimatorComponent>();
    
    int updatedCount = 0;
    for (auto entity : view) {
        auto& animComp = view.get<AnimatorComponent>(entity);
        
        // Auto-initialize if AnimatorComponent has no modelData but entity has SkinnedModelComponent
        if (!animComp.modelData && registry.all_of<SkinnedModelComponent>(entity)) {
            auto& skinnedComp = registry.get<SkinnedModelComponent>(entity);
            if (skinnedComp.model && skinnedComp.model->HasSkeleton()) {
                auto modelData = skinnedComp.model->GetModelData();
                if (modelData) {
                    animComp.Init(modelData);
                    SE_LOG_INFO("AnimationSystem: Auto-initialized AnimatorComponent from SkinnedModelComponent");
                }
            }
        }
        
        // Update animation if playing
        if (animComp.playing) {
            animComp.Update(deltaTime);
            updatedCount++;
        }
    }
    
    // Periodic debug logging
    static int frameCount = 0;
    if (frameCount < 5 || (frameCount % 300 == 0 && updatedCount > 0)) {
        SE_LOG_INFO("AnimationSystem: Updated {} animators", updatedCount);
    }
    frameCount++;
}

}  // namespace se
