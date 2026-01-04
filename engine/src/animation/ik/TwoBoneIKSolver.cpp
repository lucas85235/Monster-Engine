#include "engine/animation/ik/TwoBoneIKSolver.h"
#include "engine/animation/advanced/Pose.h"
#include "engine/resources/ModelData.h"
#include "engine/Log.h"

#include <gtx/quaternion.hpp>
#include <algorithm>
#include <cmath>

namespace se {
namespace anim {

TwoBoneIKSolver::TwoBoneIKSolver(const TwoBoneIKConfig& config)
    : config_(config) {
}

void TwoBoneIKSolver::Initialize(const SkinnedModelData* skeleton) {
    skeleton_ = skeleton;
    
    if (!skeleton_) {
        SE_LOG_ERROR("[TwoBoneIKSolver] Cannot initialize without skeleton");
        return;
    }
    
    // Find bone indices
    rootBoneIndex_ = skeleton_->GetBoneIndex(config_.rootBoneName);
    midBoneIndex_ = skeleton_->GetBoneIndex(config_.midBoneName);
    endBoneIndex_ = skeleton_->GetBoneIndex(config_.endBoneName);
    
    if (rootBoneIndex_ < 0) {
        SE_LOG_ERROR("[TwoBoneIKSolver] Root bone not found: {}", config_.rootBoneName);
        return;
    }
    if (midBoneIndex_ < 0) {
        SE_LOG_ERROR("[TwoBoneIKSolver] Mid bone not found: {}", config_.midBoneName);
        return;
    }
    if (endBoneIndex_ < 0) {
        SE_LOG_ERROR("[TwoBoneIKSolver] End bone not found: {}", config_.endBoneName);
        return;
    }
    
    SE_LOG_INFO("[TwoBoneIKSolver] Initialized: {} -> {} -> {} (indices: {}, {}, {})",
                config_.rootBoneName, config_.midBoneName, config_.endBoneName,
                rootBoneIndex_, midBoneIndex_, endBoneIndex_);
}

void TwoBoneIKSolver::Solve(Pose& pose, const IKTarget& target) {
    if (!enabled_ || !skeleton_) {
        didSolve_ = false;
        return;
    }
    
    if (rootBoneIndex_ < 0 || midBoneIndex_ < 0 || endBoneIndex_ < 0) {
        didSolve_ = false;
        return;
    }
    
    if (target.weight <= 0.001f) {
        didSolve_ = false;
        return;
    }
    
    // Get current bone positions from pose
    auto& rootTransform = pose[static_cast<size_t>(rootBoneIndex_)];
    auto& midTransform = pose[static_cast<size_t>(midBoneIndex_)];
    auto& endTransform = pose[static_cast<size_t>(endBoneIndex_)];
    
    // Calculate bone chain lengths from rest pose offset matrices
    glm::vec3 rootPos = rootTransform.position;
    glm::vec3 midPos = midTransform.position;
    glm::vec3 endPos = endTransform.position;
    
    // Use stored lengths or calculate
    if (upperLength_ <= 0.0f) {
        upperLength_ = glm::length(midPos - rootPos);
        if (upperLength_ <= 0.0f) upperLength_ = 0.3f;  // Fallback
    }
    if (lowerLength_ <= 0.0f) {
        lowerLength_ = glm::length(endPos - midPos);
        if (lowerLength_ <= 0.0f) lowerLength_ = 0.3f;  // Fallback
    }
    
    float chainLength = upperLength_ + lowerLength_;
    
    // Vector from root to target
    glm::vec3 targetDir = target.position - rootPos;
    float targetDist = glm::length(targetDir);
    
    if (targetDist < 0.001f) {
        didSolve_ = false;
        return;
    }
    
    // Clamp target distance to chain length
    reachRatio_ = targetDist / chainLength;
    float maxDist = chainLength * config_.maxStretch;
    if (targetDist > maxDist) {
        targetDist = maxDist;
    }
    
    targetDir = glm::normalize(targetDir);
    
    // Calculate bend angle using law of cosines
    // a = upper, b = lower, c = targetDist
    float cosAngle = (upperLength_ * upperLength_ + targetDist * targetDist - lowerLength_ * lowerLength_) 
                     / (2.0f * upperLength_ * targetDist);
    cosAngle = std::clamp(cosAngle, -1.0f, 1.0f);
    float rootAngle = std::acos(cosAngle);
    
    // Calculate mid joint angle
    float cosMidAngle = (upperLength_ * upperLength_ + lowerLength_ * lowerLength_ - targetDist * targetDist)
                        / (2.0f * upperLength_ * lowerLength_);
    cosMidAngle = std::clamp(cosMidAngle, -1.0f, 1.0f);
    float midAngle = std::acos(cosMidAngle);
    
    // Calculate plane for bend (using pole vector)
    glm::vec3 bendPlaneNormal = glm::cross(targetDir, config_.poleVector);
    if (glm::length(bendPlaneNormal) < 0.001f) {
        // Target is aligned with pole, use world up
        bendPlaneNormal = glm::cross(targetDir, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    bendPlaneNormal = glm::normalize(bendPlaneNormal);
    
    // Calculate rotation axis for root bone
    glm::vec3 currentToEnd = glm::normalize(endPos - rootPos);
    glm::quat rootRotation = glm::rotation(currentToEnd, targetDir);
    
    // Apply root rotation
    glm::quat originalRootRot = rootTransform.rotation;
    glm::quat newRootRot = glm::slerp(originalRootRot, rootRotation * originalRootRot, target.weight);
    rootTransform.rotation = glm::normalize(newRootRot);
    
    // Apply mid joint bend
    glm::vec3 bendAxis = glm::normalize(glm::cross(targetDir, bendPlaneNormal));
    float bendAmount = 3.14159f - midAngle;  // Convert to bend angle
    glm::quat midBendRot = glm::angleAxis(bendAmount, bendAxis);
    
    glm::quat originalMidRot = midTransform.rotation;
    glm::quat newMidRot = glm::slerp(originalMidRot, midBendRot * originalMidRot, target.weight);
    midTransform.rotation = glm::normalize(newMidRot);
    
    // Optionally apply end effector rotation
    if (target.useRotation) {
        glm::quat originalEndRot = endTransform.rotation;
        glm::quat newEndRot = glm::slerp(originalEndRot, target.rotation, target.weight);
        endTransform.rotation = glm::normalize(newEndRot);
    }
    
    didSolve_ = true;
}

glm::quat TwoBoneIKSolver::CalculateRotationToTarget(const glm::vec3& from, const glm::vec3& to,
                                                      const glm::vec3& currentDir) {
    glm::vec3 targetDir = glm::normalize(to - from);
    return glm::rotation(currentDir, targetDir);
}

}  // namespace anim
}  // namespace se
