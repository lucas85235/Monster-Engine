#pragma once

#include <glm.hpp>
#include <memory>
#include <vector>

#include "engine/animation/AnimationClip.h"
#include "engine/resources/ModelData.h"

namespace se {

// Calculates bone transforms for skeletal animation with crossfade blending support.
class Animator {
public:
    Animator() = default;
    explicit Animator(const SkinnedModelData* modelData);
    
    // Immediate playback (no blending)
    void Play(std::shared_ptr<AnimationClip> clip, bool loop = true);
    
    // Smooth crossfade transition to new clip
    void Crossfade(std::shared_ptr<AnimationClip> clip, float duration, bool loop = true);
    
    void Stop();
    void Pause();
    void Resume();
    void Update(float deltaTime);
    
    void SetSpeed(float speed) { speed_ = speed; }
    void SetTime(float time);
    float GetCurrentTime() const { return currentTime_; }
    float GetSpeed() const { return speed_; }
    
    bool IsPlaying() const { return playing_; }
    bool IsLooping() const { return looping_; }
    bool IsBlending() const { return isBlending_; }
    float GetBlendWeight() const { return blendWeight_; }
    
    const std::vector<glm::mat4>& GetBoneMatrices() const { return finalBoneMatrices_; }
    std::shared_ptr<AnimationClip> GetCurrentClip() const { return currentClip_; }
    
    // Get world-space transform for a specific bone (for attachment)
    glm::mat4 GetBoneWorldMatrix(int boneIndex) const;
    glm::mat4 GetBoneWorldMatrix(const std::string& boneName) const;
    
    void SetModelData(const SkinnedModelData* modelData);
    const SkinnedModelData* GetModelData() const { return modelData_; }
    
private:
    void CalculateBoneTransforms();
    void CalculateBoneTransformsBlended();
    void ProcessBoneHierarchy(int boneIndex, const glm::mat4& parentTransform);
    void ProcessBoneHierarchyBlended(int boneIndex, const glm::mat4& parentTransform);
    glm::mat4 GetBoneLocalTransform(const AnimationClip* clip, const std::string& boneName, float time) const;
    
    const SkinnedModelData* modelData_ = nullptr;
    
    // Current animation
    std::shared_ptr<AnimationClip> currentClip_;
    float currentTime_ = 0.0f;
    
    // Blend source (previous animation during crossfade)
    std::shared_ptr<AnimationClip> blendFromClip_;
    float blendFromTime_ = 0.0f;
    
    // Blending state
    float blendWeight_ = 1.0f;
    float blendDuration_ = 0.25f;
    bool isBlending_ = false;
    
    // Bone matrices
    std::vector<glm::mat4> finalBoneMatrices_;
    std::vector<glm::mat4> localTransforms_;
    
    // Playback state
    float speed_ = 1.0f;
    bool playing_ = false;
    bool looping_ = true;
    bool paused_ = false;
};

}  // namespace se
