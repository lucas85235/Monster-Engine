#include "engine/animation/advanced/LookAtController.h"

#include "engine/animation/advanced/Pose.h"
#include "engine/resources/ModelData.h"
#include "engine/Log.h"

#include <cmath>

namespace se::anim {

LookAtSettings LookAtSettings::DefaultMixamo() {
    LookAtSettings settings;
    settings.smoothSpeed = 10.0f;
    settings.horizontalLimit = 90.0f;
    settings.verticalLimit = 60.0f;
    settings.deadzone = 5.0f;
    settings.enabled = true;
    
    // Mixamo skeleton bone chain with distributed weights
    settings.boneChain = {
        {"mixamorig:Spine1", 0.15f, 0.10f, 20.0f, 15.0f},
        {"mixamorig:Spine2", 0.20f, 0.15f, 25.0f, 20.0f},
        {"mixamorig:Spine",  0.00f, 0.00f, 0.0f, 0.0f},  // Optional, might not exist
        {"mixamorig:Neck",   0.25f, 0.25f, 35.0f, 30.0f},
        {"mixamorig:Head",   0.15f, 0.25f, 40.0f, 35.0f}
    };
    
    return settings;
}

void LookAtController::Initialize(const SkinnedModelData* skeleton, const LookAtSettings& settings) {
    skeleton_ = skeleton;
    settings_ = settings;
    resolvedBones_.clear();
    initialized_ = false;
    
    if (!skeleton) {
        SE_LOG_WARN("[LookAtController] Cannot initialize without skeleton");
        return;
    }
    
    // Resolve bone indices
    for (const auto& boneSettings : settings_.boneChain) {
        int boneIndex = skeleton->GetBoneIndex(boneSettings.boneName);
        
        if (boneIndex >= 0) {
            ResolvedBone resolved;
            resolved.index = boneIndex;
            resolved.horizontalWeight = boneSettings.horizontalWeight;
            resolved.verticalWeight = boneSettings.verticalWeight;
            resolved.maxHorizontal = glm::radians(boneSettings.maxHorizontalDegrees);
            resolved.maxVertical = glm::radians(boneSettings.maxVerticalDegrees);
            resolvedBones_.push_back(resolved);
        }
    }
    
    if (resolvedBones_.empty()) {
        SE_LOG_WARN("[LookAtController] No bones found in skeleton for look-at chain");
        return;
    }
    
    initialized_ = true;
    SE_LOG_INFO("[LookAtController] Initialized with {} bones", resolvedBones_.size());
}

void LookAtController::SetTarget(const glm::vec3& worldDirection, const glm::vec3& characterForward, const glm::vec3& characterUp) {
    if (!settings_.enabled) {
        targetAngles_ = glm::vec2(0.0f);
        return;
    }
    
    // Project direction onto horizontal plane
    glm::vec3 dirNorm = glm::normalize(worldDirection);
    glm::vec3 forwardNorm = glm::normalize(characterForward);
    glm::vec3 upNorm = glm::normalize(characterUp);
    glm::vec3 rightNorm = glm::normalize(glm::cross(upNorm, forwardNorm));
    
    // Calculate horizontal angle (yaw)
    glm::vec3 dirHorizontal = glm::normalize(dirNorm - upNorm * glm::dot(dirNorm, upNorm));
    float horizontalAngle = std::acos(glm::clamp(glm::dot(dirHorizontal, forwardNorm), -1.0f, 1.0f));
    
    // Determine sign
    if (glm::dot(dirHorizontal, rightNorm) < 0.0f) {
        horizontalAngle = -horizontalAngle;
    }
    
    // Calculate vertical angle (pitch)
    float verticalAngle = std::asin(glm::clamp(glm::dot(dirNorm, upNorm), -1.0f, 1.0f));
    
    // Convert to degrees and clamp
    float horizontalDegrees = glm::degrees(horizontalAngle);
    float verticalDegrees = glm::degrees(verticalAngle);
    
    SetTargetAngles(horizontalDegrees, verticalDegrees);
}

void LookAtController::SetTargetAngles(float horizontalDegrees, float verticalDegrees) {
    // Apply deadzone
    if (std::abs(horizontalDegrees) < settings_.deadzone) {
        horizontalDegrees = 0.0f;
    }
    if (std::abs(verticalDegrees) < settings_.deadzone) {
        verticalDegrees = 0.0f;
    }
    
    // Clamp to limits
    targetAngles_.x = glm::clamp(horizontalDegrees, -settings_.horizontalLimit, settings_.horizontalLimit);
    targetAngles_.y = glm::clamp(verticalDegrees, -settings_.verticalLimit, settings_.verticalLimit);
}

void LookAtController::Update(float dt) {
    if (!settings_.enabled) {
        // Smoothly return to center
        targetAngles_ = glm::vec2(0.0f);
    }
    
    // Smooth interpolation
    float t = 1.0f - std::exp(-settings_.smoothSpeed * dt);
    currentAngles_ = glm::mix(currentAngles_, targetAngles_, t);
    
    // Snap to zero if very close
    if (glm::length(currentAngles_) < 0.1f) {
        currentAngles_ = glm::vec2(0.0f);
    }
}

void LookAtController::ApplyToPose(Pose& pose) {
    if (!initialized_ || !settings_.enabled || pose.IsEmpty()) {
        return;
    }
    
    // Convert current angles to radians
    float horizontalRad = glm::radians(currentAngles_.x);
    float verticalRad = glm::radians(currentAngles_.y);
    
    // Apply rotation to each bone in chain
    for (const auto& bone : resolvedBones_) {
        if (bone.index < 0 || bone.index >= static_cast<int>(pose.GetBoneCount())) {
            continue;
        }
        
        // Calculate per-bone rotation
        float boneHorizontal = horizontalRad * bone.horizontalWeight;
        float boneVertical = verticalRad * bone.verticalWeight;
        
        // Clamp to bone limits
        boneHorizontal = glm::clamp(boneHorizontal, -bone.maxHorizontal, bone.maxHorizontal);
        boneVertical = glm::clamp(boneVertical, -bone.maxVertical, bone.maxVertical);
        
        // Create rotation quaternions
        // Horizontal (Y axis) and Vertical (X axis)
        glm::quat horizontalRot = glm::angleAxis(boneHorizontal, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat verticalRot = glm::angleAxis(-boneVertical, glm::vec3(1.0f, 0.0f, 0.0f));
        
        // Combined rotation (horizontal first, then vertical)
        glm::quat lookAtRot = horizontalRot * verticalRot;
        
        // Apply as additive rotation
        auto& boneTransform = pose[static_cast<size_t>(bone.index)];
        boneTransform.rotation = glm::normalize(boneTransform.rotation * lookAtRot);
    }
}

}  // namespace se::anim
