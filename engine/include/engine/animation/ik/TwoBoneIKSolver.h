#pragma once

#include "engine/animation/ik/IIKSolver.h"
#include <string>

namespace se {
namespace anim {

struct TwoBoneIKConfig {
    std::string rootBoneName;   // e.g., "upperarm_l"
    std::string midBoneName;    // e.g., "lowerarm_l"
    std::string endBoneName;    // e.g., "hand_l"
    glm::vec3 poleVector{0.0f, 0.0f, -1.0f};  // Elbow/knee bend direction
    float maxStretch = 1.0f;    // Allow stretching beyond full extension (1.0 = no stretch)
};

class TwoBoneIKSolver : public IIKSolver {
public:
    TwoBoneIKSolver() = default;
    explicit TwoBoneIKSolver(const TwoBoneIKConfig& config);
    
    void Initialize(const SkinnedModelData* skeleton) override;
    void Solve(Pose& pose, const IKTarget& target) override;
    std::string GetName() const override { return "TwoBoneIK"; }
    
    void SetConfig(const TwoBoneIKConfig& config) { config_ = config; }
    const TwoBoneIKConfig& GetConfig() const { return config_; }
    
    // Set pole vector dynamically
    void SetPoleVector(const glm::vec3& pole) { config_.poleVector = pole; }
    
    // Debug info
    bool DidSolve() const { return didSolve_; }
    float GetReachRatio() const { return reachRatio_; }
    
private:
    glm::quat CalculateRotationToTarget(const glm::vec3& from, const glm::vec3& to, 
                                         const glm::vec3& currentDir);
    
    TwoBoneIKConfig config_;
    const SkinnedModelData* skeleton_ = nullptr;
    
    int rootBoneIndex_ = -1;
    int midBoneIndex_ = -1;
    int endBoneIndex_ = -1;
    
    float upperLength_ = 0.0f;
    float lowerLength_ = 0.0f;
    
    bool didSolve_ = false;
    float reachRatio_ = 0.0f;
};

}  // namespace anim
}  // namespace se
