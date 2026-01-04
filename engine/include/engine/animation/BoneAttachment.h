#pragma once

#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <string>

namespace se {

class SkinnedModelData;
class Animator;
class Entity;

namespace anim {

struct BoneAttachmentConfig {
    std::string boneName;
    glm::vec3 positionOffset{0.0f};
    glm::vec3 rotationOffset{0.0f};  // Euler degrees
    glm::vec3 scale{1.0f};
    bool inheritScale = true;
};

class BoneAttachment {
public:
    BoneAttachment() = default;
    explicit BoneAttachment(const BoneAttachmentConfig& config);
    
    void Initialize(const BoneAttachmentConfig& config, const SkinnedModelData* skeleton);
    bool IsInitialized() const { return boneIndex_ >= 0; }
    
    void Update(const Animator* animator, const glm::mat4& characterWorldMatrix);
    
    glm::mat4 GetWorldTransform() const { return worldTransform_; }
    glm::vec3 GetWorldPosition() const;
    glm::quat GetWorldRotation() const;
    
    void ApplyToEntity(Entity& entity);
    
    void SetConfig(const BoneAttachmentConfig& config);
    const BoneAttachmentConfig& GetConfig() const { return config_; }
    
    void SetPositionOffset(const glm::vec3& offset) { config_.positionOffset = offset; }
    void SetRotationOffset(const glm::vec3& offset) { config_.rotationOffset = offset; }
    void SetScale(const glm::vec3& scale) { config_.scale = scale; }
    
    int GetBoneIndex() const { return boneIndex_; }
    const std::string& GetBoneName() const { return config_.boneName; }
    
private:
    BoneAttachmentConfig config_;
    const SkinnedModelData* skeleton_ = nullptr;
    int boneIndex_ = -1;
    glm::mat4 worldTransform_{1.0f};
};

}  // namespace anim
}  // namespace se
