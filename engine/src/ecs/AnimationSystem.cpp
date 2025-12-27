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

void AnimationSystem::Update(Scene& scene, float deltaTime) {
    auto& registry = scene.GetRegistry();
    
    auto view = registry.view<AnimatorComponent>();
    
    for (auto entity : view) {
        auto& animComp = view.get<AnimatorComponent>(entity);
        
        // Auto-initialize if AnimatorComponent has no modelData but entity has SkinnedModelComponent
        if (!animComp.modelData && registry.all_of<SkinnedModelComponent>(entity)) {
            auto& skinnedComp = registry.get<SkinnedModelComponent>(entity);
            if (skinnedComp.model && skinnedComp.model->HasSkeleton()) {
                auto modelData = skinnedComp.model->GetModelData();
                if (modelData) {
                    animComp.Init(modelData);
                }
            }
        }
        
        // Update animation if playing
        if (animComp.playing) {
            animComp.Update(deltaTime);
        }
    }
    
    // Update bone attachments after animations are processed
    UpdateBoneAttachments(scene);
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
        AnimatorComponent* animComp = nullptr;
        
        // First try direct entity
        if (registry.all_of<AnimatorComponent>(attachment.TargetEntity.GetHandle())) {
            animComp = &registry.get<AnimatorComponent>(attachment.TargetEntity.GetHandle());
        }
        
        if (!animComp || !animComp->animator) {
            continue;
        }
        
        // Get bone world matrix
        glm::mat4 boneWorld = animComp->animator->GetBoneWorldMatrix(attachment.BoneName);
        
        // Get target entity's world transform to combine with bone
        glm::mat4 targetWorld(1.0f);
        if (registry.all_of<TransformComponent>(attachment.TargetEntity.GetHandle())) {
            auto& targetTransform = registry.get<TransformComponent>(attachment.TargetEntity.GetHandle());
            targetWorld = targetTransform.WorldMatrix;
        }
        
        // Calculate final world matrix: TargetWorld * BoneWorld * Offset
        glm::mat4 offsetRotation = glm::toMat4(glm::quat(glm::radians(attachment.RotationOffset)));
        glm::mat4 offsetTranslation = glm::translate(glm::mat4(1.0f), attachment.PositionOffset);
        glm::mat4 offsetScale = glm::scale(glm::mat4(1.0f), attachment.ScaleMultiplier);
        glm::mat4 offsetMatrix = offsetTranslation * offsetRotation * offsetScale;
        
        glm::mat4 finalWorld = targetWorld * boneWorld * offsetMatrix;
        
        // Decompose and apply to transform
        glm::vec3 scale, translation, skew;
        glm::vec4 perspective;
        glm::quat rotation;
        glm::decompose(finalWorld, scale, rotation, translation, skew, perspective);
        
        transform.SetPosition(translation);
        transform.SetRotation(glm::degrees(glm::eulerAngles(rotation)));
        transform.SetScale(scale);
    }
}

}  // namespace se
