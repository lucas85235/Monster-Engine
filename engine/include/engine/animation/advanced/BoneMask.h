#pragma once
/**
 * BoneMask.h - Efficient bone masking system for animation layers.
 * 
 * Uses bitset for fast bone inclusion checks and supports soft weights
 * for smooth layer boundaries.
 */

#include <bitset>
#include <array>
#include <string>
#include <vector>

namespace se {

class SkinnedModelData;

namespace anim {

static constexpr size_t MAX_BONES = 128;

struct BoneMaskData {
    std::string name;
    std::vector<std::string> includedBones;
    std::vector<float> boneWeights;
};

class BoneMask {
public:
    BoneMask();
    
    void Clear();
    
    void Include(int boneIndex, float weight = 1.0f);
    void Exclude(int boneIndex);
    
    bool Contains(int boneIndex) const;
    float GetWeight(int boneIndex) const;
    
    void SetWeight(int boneIndex, float weight);
    
    void IncludeAll();
    
    static BoneMask FromBoneAndChildren(const SkinnedModelData* skeleton, const std::string& rootBone);
    
    static BoneMask FullBody(const SkinnedModelData* skeleton);
    static BoneMask UpperBody(const SkinnedModelData* skeleton);
    static BoneMask LowerBody(const SkinnedModelData* skeleton);
    static BoneMask SpineChain(const SkinnedModelData* skeleton);
    static BoneMask LeftArm(const SkinnedModelData* skeleton);
    static BoneMask RightArm(const SkinnedModelData* skeleton);
    
    void FromData(const BoneMaskData& data, const SkinnedModelData* skeleton);
    BoneMaskData ToData(const SkinnedModelData* skeleton) const;
    
    size_t CountIncluded() const { return mask_.count(); }
    
private:
    void IncludeChildrenRecursive(const SkinnedModelData* skeleton, int boneIndex, float weight);
    
    std::bitset<MAX_BONES> mask_;
    std::array<float, MAX_BONES> weights_;
};

}  // namespace anim
}  // namespace se
