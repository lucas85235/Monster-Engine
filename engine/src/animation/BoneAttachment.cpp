#include "engine/animation/BoneAttachment.h"
#include "engine/animation/Animator.h"
#include "engine/resources/ModelData.h"
#include "engine/ecs/Entity.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/Log.h"

#include <gtx/matrix_decompose.hpp>
#include <gtx/euler_angles.hpp>

namespace se {
namespace anim {

BoneAttachment::BoneAttachment(const BoneAttachmentConfig& config)
    : config_(config) {
}

void BoneAttachment::Initialize(const BoneAttachmentConfig& config, const SkinnedModelData* skeleton) {
    config_ = config;
    skeleton_ = skeleton;
    
    if (!skeleton_) {
        SE_LOG_ERROR("[BoneAttachment] Cannot initialize without skeleton");
        boneIndex_ = -1;
        return;
    }
    
    boneIndex_ = skeleton_->GetBoneIndex(config_.boneName);
    
    if (boneIndex_ < 0) {
        SE_LOG_WARN("[BoneAttachment] Bone not found: {}", config_.boneName);
    } else {
        SE_LOG_INFO("[BoneAttachment] Attached to bone '{}' (index {})", config_.boneName, boneIndex_);
    }
}

void BoneAttachment::SetConfig(const BoneAttachmentConfig& config) {
    bool needsReinit = (config.boneName != config_.boneName);
    config_ = config;
    
    if (needsReinit && skeleton_) {
        boneIndex_ = skeleton_->GetBoneIndex(config_.boneName);
    }
}

void BoneAttachment::Update(const Animator* animator, const glm::mat4& characterWorldMatrix) {
    if (!animator || boneIndex_ < 0) {
        return;
    }
    
    // Get bone world matrix from animator
    glm::mat4 boneWorld = animator->GetBoneWorldMatrix(boneIndex_);
    
    // Apply character world transform
    glm::mat4 finalBoneWorld = characterWorldMatrix * boneWorld;
    
    // Apply offset transform
    glm::mat4 offsetTranslation = glm::translate(glm::mat4(1.0f), config_.positionOffset);
    glm::mat4 offsetRotation = glm::eulerAngleXYZ(
        glm::radians(config_.rotationOffset.x),
        glm::radians(config_.rotationOffset.y),
        glm::radians(config_.rotationOffset.z)
    );
    glm::mat4 offsetScale = glm::scale(glm::mat4(1.0f), config_.scale);
    
    // Order: translate, then rotate in bone space
    glm::mat4 offsetMatrix = offsetTranslation * offsetRotation * offsetScale;
    
    worldTransform_ = finalBoneWorld * offsetMatrix;
}

glm::vec3 BoneAttachment::GetWorldPosition() const {
    return glm::vec3(worldTransform_[3]);
}

glm::quat BoneAttachment::GetWorldRotation() const {
    glm::vec3 scale, translation, skew;
    glm::quat rotation;
    glm::vec4 perspective;
    glm::decompose(worldTransform_, scale, rotation, translation, skew, perspective);
    return rotation;
}

void BoneAttachment::ApplyToEntity(Entity& entity) {
    if (!entity.IsValid()) return;
    
    auto& transform = entity.GetComponent<TransformComponent>();
    
    // Decompose world transform
    glm::vec3 scale, translation, skew;
    glm::quat rotation;
    glm::vec4 perspective;
    glm::decompose(worldTransform_, scale, rotation, translation, skew, perspective);
    
    // Apply to entity transform
    transform.SetPosition(translation);
    transform.SetRotation(glm::degrees(glm::eulerAngles(rotation)));
    
    if (config_.inheritScale) {
        transform.SetScale(scale);
    } else {
        transform.SetScale(config_.scale);
    }
}

}  // namespace anim
}  // namespace se
