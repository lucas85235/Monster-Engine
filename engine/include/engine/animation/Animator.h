#pragma once

#include <glm.hpp>
#include <memory>
#include <vector>

#include "engine/animation/AnimationClip.h"
#include "engine/resources/ModelData.h"

namespace se {

// Calculates bone transforms for skeletal animation
class Animator {
public:
    Animator() = default;
    explicit Animator(const SkinnedModelData* modelData);
    
    void Play(std::shared_ptr<AnimationClip> clip, bool loop = true);
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
    
    const std::vector<glm::mat4>& GetBoneMatrices() const { return finalBoneMatrices_; }
    std::shared_ptr<AnimationClip> GetCurrentClip() const { return currentClip_; }
    
    void SetModelData(const SkinnedModelData* modelData);
    
private:
    void CalculateBoneTransforms();
    void ProcessBoneHierarchy(int boneIndex, const glm::mat4& parentTransform);
    glm::mat4 GetBoneLocalTransform(const std::string& boneName, float time) const;
    
    const SkinnedModelData* modelData_ = nullptr;
    std::shared_ptr<AnimationClip> currentClip_;
    
    std::vector<glm::mat4> finalBoneMatrices_;
    std::vector<glm::mat4> localTransforms_;
    
    float currentTime_ = 0.0f;
    float speed_ = 1.0f;
    bool playing_ = false;
    bool looping_ = true;
    bool paused_ = false;
};

}  // namespace se
