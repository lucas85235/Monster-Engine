#pragma once
/**
 * Pose.h - Fundamental data structures for animation blending.
 * 
 * BoneTransform represents a single bone's local transform.
 * Pose represents a complete skeleton pose for blending operations.
 */

#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <vector>
#include <string>

namespace se {

class SkinnedModelData;
class AnimationClip;

namespace anim {

struct BoneTransform {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
    
    static BoneTransform Identity() {
        return BoneTransform{};
    }
    
    static BoneTransform Blend(const BoneTransform& a, const BoneTransform& b, float t);
    static BoneTransform BlendAdditive(const BoneTransform& base, const BoneTransform& additive, float weight);
};

class Pose {
public:
    Pose() = default;
    explicit Pose(size_t boneCount);
    explicit Pose(const SkinnedModelData* skeleton);
    
    void Resize(size_t boneCount);
    void SetIdentity();
    
    void SetFromClip(const AnimationClip* clip, float time, const SkinnedModelData* skeleton);
    
    void BlendWith(const Pose& other, float weight);
    
    void ApplyAdditive(const Pose& additive, float weight);
    
    BoneTransform& operator[](size_t index) { return transforms_[index]; }
    const BoneTransform& operator[](size_t index) const { return transforms_[index]; }
    
    size_t GetBoneCount() const { return transforms_.size(); }
    bool IsEmpty() const { return transforms_.empty(); }
    
    const std::vector<BoneTransform>& GetTransforms() const { return transforms_; }
    std::vector<BoneTransform>& GetTransforms() { return transforms_; }

private:
    std::vector<BoneTransform> transforms_;
};

}  // namespace anim
}  // namespace se
