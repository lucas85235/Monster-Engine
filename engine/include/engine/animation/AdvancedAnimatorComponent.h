#pragma once

#include "engine/animation/graph/AnimationGraph.h"
#include "engine/animation/locomotion/LocomotionController.h"
#include "engine/animation/advanced/LookAtController.h"
#include "engine/animation/advanced/BodyRotationController.h"
#include "engine/animation/advanced/AnimationLayer.h"
#include "engine/animation/advanced/Pose.h"
#include "engine/animation/BoneAttachment.h"
#include "engine/animation/ik/IIKSolver.h"

#include <memory>
#include <vector>

namespace se {

class SkinnedModelData;
class Animator;

namespace anim {

struct AdvancedAnimatorConfig {
    // Model configuration
    std::string modelPath;
    glm::vec3 modelScale{0.01f};
    glm::vec3 modelRotation{0.0f};
    glm::vec3 modelOffset{0.0f};
    
    // Locomotion
    LocomotionConfig locomotion;
    
    // Look-at
    LookAtSettings lookAt = LookAtSettings::DefaultUEMannequin();
    
    // Body rotation
    BodyRotationSettings bodyRotation = BodyRotationSettings::Default();
};

class AdvancedAnimatorComponent {
public:
    AdvancedAnimatorComponent() = default;
    
    void Initialize(const AdvancedAnimatorConfig& config, const SkinnedModelData* skeleton);
    bool IsInitialized() const { return initialized_; }
    
    void Update(float dt, Animator* animator);
    
    // Animation graph access
    AnimationGraph* GetGraph() { return graph_.get(); }
    const AnimationGraph* GetGraph() const { return graph_.get(); }
    void SetGraph(std::unique_ptr<AnimationGraph> graph);
    
    // Locomotion
    LocomotionController* GetLocomotion() { return &locomotion_; }
    const LocomotionController* GetLocomotion() const { return &locomotion_; }
    
    // Procedural animation
    LookAtController* GetLookAt() { return lookAt_.get(); }
    BodyRotationController* GetBodyRotation() { return bodyRotation_.get(); }
    
    // Layer stack
    AnimationLayerStack& GetLayers() { return layers_; }
    const AnimationLayerStack& GetLayers() const { return layers_; }
    
    // IK solvers
    void AddIKSolver(std::unique_ptr<IIKSolver> solver);
    IIKSolver* GetIKSolver(const std::string& name);
    const std::vector<std::unique_ptr<IIKSolver>>& GetIKSolvers() const { return ikSolvers_; }
    
    // Bone attachments
    size_t AddAttachment(const BoneAttachmentConfig& config);
    BoneAttachment* GetAttachment(size_t index);
    BoneAttachment* GetAttachment(const std::string& boneName);
    const std::vector<BoneAttachment>& GetAttachments() const { return attachments_; }
    
    // Final pose
    const Pose& GetFinalPose() const { return finalPose_; }
    Pose& GetFinalPose() { return finalPose_; }
    
    // Configuration
    AdvancedAnimatorConfig& GetConfig() { return config_; }
    const AdvancedAnimatorConfig& GetConfig() const { return config_; }
    
private:
    void UpdateLocomotion(float dt);
    void UpdateProceduralAnimation(float dt);
    void UpdateIK();
    void UpdateLayers();
    void UpdateAttachments(Animator* animator, const glm::mat4& worldMatrix);
    void ApplyFinalPose(Animator* animator);
    
    AdvancedAnimatorConfig config_;
    const SkinnedModelData* skeleton_ = nullptr;
    bool initialized_ = false;
    
    // Core systems
    std::unique_ptr<AnimationGraph> graph_;
    LocomotionController locomotion_;
    
    // Procedural
    std::unique_ptr<LookAtController> lookAt_;
    std::unique_ptr<BodyRotationController> bodyRotation_;
    
    // Layers
    AnimationLayerStack layers_;
    
    // IK
    std::vector<std::unique_ptr<IIKSolver>> ikSolvers_;
    
    // Attachments
    std::vector<BoneAttachment> attachments_;
    
    // Poses
    Pose basePose_;
    Pose finalPose_;
};

}  // namespace anim
}  // namespace se
