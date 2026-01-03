#include "engine/animation/advanced/BoneMask.h"

#include "engine/resources/ModelData.h"
#include "engine/Log.h"

#include <algorithm>

namespace se::anim {

BoneMask::BoneMask() {
    Clear();
}

void BoneMask::Clear() {
    mask_.reset();
    weights_.fill(0.0f);
}

void BoneMask::Include(int boneIndex, float weight) {
    if (boneIndex < 0 || boneIndex >= static_cast<int>(MAX_BONES)) {
        return;
    }
    mask_.set(static_cast<size_t>(boneIndex));
    weights_[static_cast<size_t>(boneIndex)] = weight;
}

void BoneMask::Exclude(int boneIndex) {
    if (boneIndex < 0 || boneIndex >= static_cast<int>(MAX_BONES)) {
        return;
    }
    mask_.reset(static_cast<size_t>(boneIndex));
    weights_[static_cast<size_t>(boneIndex)] = 0.0f;
}

bool BoneMask::Contains(int boneIndex) const {
    if (boneIndex < 0 || boneIndex >= static_cast<int>(MAX_BONES)) {
        return false;
    }
    return mask_.test(static_cast<size_t>(boneIndex));
}

float BoneMask::GetWeight(int boneIndex) const {
    if (boneIndex < 0 || boneIndex >= static_cast<int>(MAX_BONES)) {
        return 0.0f;
    }
    return weights_[static_cast<size_t>(boneIndex)];
}

void BoneMask::SetWeight(int boneIndex, float weight) {
    if (boneIndex < 0 || boneIndex >= static_cast<int>(MAX_BONES)) {
        return;
    }
    weights_[static_cast<size_t>(boneIndex)] = weight;
    if (weight > 0.0f) {
        mask_.set(static_cast<size_t>(boneIndex));
    }
}

void BoneMask::IncludeAll() {
    mask_.set();
    weights_.fill(1.0f);
}

void BoneMask::IncludeChildrenRecursive(const SkinnedModelData* skeleton, int boneIndex, float weight) {
    if (!skeleton || boneIndex < 0 || boneIndex >= static_cast<int>(skeleton->Bones.size())) {
        return;
    }
    
    Include(boneIndex, weight);
    
    // Find children
    for (size_t i = 0; i < skeleton->Bones.size(); ++i) {
        const auto& bone = skeleton->Bones[static_cast<size_t>(i)];
        if (bone.ParentIndex == boneIndex) {
            IncludeChildrenRecursive(skeleton, static_cast<int>(i), weight);
        }
    }
}

BoneMask BoneMask::FromBoneAndChildren(const SkinnedModelData* skeleton, const std::string& rootBone) {
    BoneMask mask;
    if (!skeleton) {
        return mask;
    }
    
    int rootIndex = skeleton->GetBoneIndex(rootBone);
    if (rootIndex >= 0) {
        mask.IncludeChildrenRecursive(skeleton, rootIndex, 1.0f);
    } else {
        SE_LOG_WARN("[BoneMask] Bone '{}' not found in skeleton", rootBone);
    }
    
    return mask;
}

BoneMask BoneMask::FullBody(const SkinnedModelData* skeleton) {
    BoneMask mask;
    if (skeleton) {
        for (size_t i = 0; i < skeleton->Bones.size(); ++i) {
            mask.Include(static_cast<int>(i), 1.0f);
        }
    }
    return mask;
}

BoneMask BoneMask::UpperBody(const SkinnedModelData* skeleton) {
    // Mixamo skeleton: Spine1 and above
    static const std::vector<std::string> rootBones = {
        "mixamorig:Spine1",
        "Spine1",
        "spine_01"
    };
    
    for (const auto& boneName : rootBones) {
        BoneMask mask = FromBoneAndChildren(skeleton, boneName);
        if (mask.CountIncluded() > 0) {
            return mask;
        }
    }
    
    SE_LOG_WARN("[BoneMask] UpperBody: Could not find spine root bone");
    return BoneMask();
}

BoneMask BoneMask::LowerBody(const SkinnedModelData* skeleton) {
    // Mixamo skeleton: Hips and below (excluding spine)
    BoneMask mask;
    if (!skeleton) {
        return mask;
    }
    
    static const std::vector<std::string> hipBones = {
        "mixamorig:Hips",
        "Hips",
        "pelvis"
    };
    
    int hipsIndex = -1;
    for (const auto& boneName : hipBones) {
        hipsIndex = skeleton->GetBoneIndex(boneName);
        if (hipsIndex >= 0) break;
    }
    
    if (hipsIndex < 0) {
        SE_LOG_WARN("[BoneMask] LowerBody: Could not find hips bone");
        return mask;
    }
    
    // Include hips
    mask.Include(hipsIndex, 1.0f);
    
    // Find and include leg chains
    static const std::vector<std::string> legRoots = {
        "mixamorig:LeftUpLeg", "mixamorig:RightUpLeg",
        "LeftUpLeg", "RightUpLeg",
        "thigh_l", "thigh_r"
    };
    
    for (const auto& legBone : legRoots) {
        int legIndex = skeleton->GetBoneIndex(legBone);
        if (legIndex >= 0) {
            mask.IncludeChildrenRecursive(skeleton, legIndex, 1.0f);
        }
    }
    
    return mask;
}

BoneMask BoneMask::SpineChain(const SkinnedModelData* skeleton) {
    BoneMask mask;
    if (!skeleton) {
        return mask;
    }
    
    static const std::vector<std::string> spineBones = {
        "mixamorig:Spine", "mixamorig:Spine1", "mixamorig:Spine2",
        "mixamorig:Neck", "mixamorig:Head",
        "Spine", "Spine1", "Spine2", "Neck", "Head",
        "spine_01", "spine_02", "spine_03", "neck_01", "head"
    };
    
    for (const auto& boneName : spineBones) {
        int index = skeleton->GetBoneIndex(boneName);
        if (index >= 0) {
            mask.Include(index, 1.0f);
        }
    }
    
    return mask;
}

BoneMask BoneMask::LeftArm(const SkinnedModelData* skeleton) {
    static const std::vector<std::string> armRoots = {
        "mixamorig:LeftShoulder",
        "LeftShoulder",
        "clavicle_l"
    };
    
    for (const auto& boneName : armRoots) {
        BoneMask mask = FromBoneAndChildren(skeleton, boneName);
        if (mask.CountIncluded() > 0) {
            return mask;
        }
    }
    
    return BoneMask();
}

BoneMask BoneMask::RightArm(const SkinnedModelData* skeleton) {
    static const std::vector<std::string> armRoots = {
        "mixamorig:RightShoulder",
        "RightShoulder",
        "clavicle_r"
    };
    
    for (const auto& boneName : armRoots) {
        BoneMask mask = FromBoneAndChildren(skeleton, boneName);
        if (mask.CountIncluded() > 0) {
            return mask;
        }
    }
    
    return BoneMask();
}

void BoneMask::FromData(const BoneMaskData& data, const SkinnedModelData* skeleton) {
    Clear();
    
    if (!skeleton) {
        return;
    }
    
    for (size_t i = 0; i < data.includedBones.size(); ++i) {
        int boneIndex = skeleton->GetBoneIndex(data.includedBones[i]);
        if (boneIndex >= 0) {
            float weight = (i < data.boneWeights.size()) ? data.boneWeights[i] : 1.0f;
            Include(boneIndex, weight);
        }
    }
}

BoneMaskData BoneMask::ToData(const SkinnedModelData* skeleton) const {
    BoneMaskData data;
    
    if (!skeleton) {
        return data;
    }
    
    for (size_t i = 0; i < skeleton->Bones.size() && i < MAX_BONES; ++i) {
        if (mask_.test(i)) {
            data.includedBones.push_back(skeleton->Bones[i].Name);
            data.boneWeights.push_back(weights_[i]);
        }
    }
    
    return data;
}

}  // namespace se::anim
