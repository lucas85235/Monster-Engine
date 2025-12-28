#include "engine/ecs/AnimationSystem.h"

#include <gtc/matrix_transform.hpp>
#include <gtx/quaternion.hpp>
#include <gtx/matrix_decompose.hpp>

#include "engine/Log.h"
#include "engine/ecs/AnimatorComponent.h"
#include "engine/ecs/BoneAttachmentComponent.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/ecs/SkinnedModelComponent.h"

namespace se {

// Fixed timestep for animation (60Hz)
static constexpr float ANIMATION_TIMESTEP = 1.0f / 60.0f;
static float s_animAccumulator = 0.0f;

void AnimationSystem::Update(Scene& scene, float deltaTime) {
    auto& registry = scene.GetRegistry();
    
    auto view = registry.view<AnimatorComponent>();
    
    // Auto-initialize any new AnimatorComponents
    for (auto entity : view) {
        auto& animComp = view.get<AnimatorComponent>(entity);
        
        if (!animComp.modelData && registry.all_of<SkinnedModelComponent>(entity)) {
            auto& skinnedComp = registry.get<SkinnedModelComponent>(entity);
            if (skinnedComp.model && skinnedComp.model->HasSkeleton()) {
                auto modelData = skinnedComp.model->GetModelData();
                if (modelData) {
                    animComp.Init(modelData);
                }
            }
        }
    }
    
    // Fixed timestep animation update (60Hz)
    s_animAccumulator += deltaTime;
    
    while (s_animAccumulator >= ANIMATION_TIMESTEP) {
        for (auto entity : view) {
            auto& animComp = view.get<AnimatorComponent>(entity);
            if (animComp.playing) {
                animComp.Update(ANIMATION_TIMESTEP);
            }
        }
        s_animAccumulator -= ANIMATION_TIMESTEP;
    }
}

void AnimationSystem::UpdateBoneAttachments(Scene& scene) {
    auto& registry = scene.GetRegistry();
    
    auto view = registry.view<BoneAttachmentComponent, TransformComponent>();
    
    for (auto entity : view) {
        auto& attachment = view.get<BoneAttachmentComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);
        
        if (!attachment.Active || !attachment.TargetEntity.IsValid()) {
            continue;
        }
        
        // Get target entity's AnimatorComponent (may be on visual child)
        AnimatorComponent* animComp = registry.try_get<AnimatorComponent>(attachment.TargetEntity.GetHandle());
        
        if (!animComp || !animComp->animator) {
            static std::unordered_map<uint32_t, bool> warnedEntities;
            if (warnedEntities.find((uint32_t)entity) == warnedEntities.end()) {
                SE_LOG_WARN("AnimationSystem: Attached target entity has no AnimatorComponent!");
                warnedEntities[(uint32_t)entity] = true;
            }
            continue;
        }
        
        // 1. Get bone transform relative to character model root
        glm::mat4 boneModelSpace = animComp->animator->GetBoneWorldMatrix(attachment.BoneName);
        
        // 2. Get character world transform
        TransformComponent* targetTransform = registry.try_get<TransformComponent>(attachment.TargetEntity.GetHandle());
        glm::mat4 targetWorld = targetTransform ? targetTransform->WorldMatrix : glm::mat4(1.0f);
        
        // 3. Decompose transforms to handle them separately (avoid scale inheritance issues)
        
        // Extract translation from combined matrix (this is the bone's world position)
        glm::mat4 boneFullWorld = targetWorld * boneModelSpace;
        glm::vec3 worldPos = glm::vec3(boneFullWorld[3]);
        
        // Extract rotation: Normalize columns of targetWorld * boneModelSpace to get pure rotation
        glm::vec3 col0 = glm::normalize(glm::vec3(boneFullWorld[0]));
        glm::vec3 col1 = glm::normalize(glm::vec3(boneFullWorld[1]));
        glm::vec3 col2 = glm::normalize(glm::vec3(boneFullWorld[2]));
        glm::mat3 boneWorldRotMat(col0, col1, col2);
        glm::quat boneWorldRotation = glm::quat_cast(boneWorldRotMat);
        
        // 4. Apply offsets
        // Final position: Bone world position + (Bone world rotation * offset)
        glm::vec3 finalWorldPos = worldPos + (boneWorldRotation * attachment.PositionOffset);
        
        // Final rotation: Bone world rotation * Offset rotation
        glm::quat offsetRot = glm::quat(glm::radians(attachment.RotationOffset));
        glm::quat finalWorldRot = boneWorldRotation * offsetRot;
        
        // 5. Apply to transform
        transform.SetPosition(finalWorldPos);
        transform.SetRotation(glm::degrees(glm::eulerAngles(finalWorldRot)));
        
        // Force ScaleMultiplier as absolute world scale for root entities
        transform.SetScale(attachment.ScaleMultiplier);
    }
}

}  // namespace se
