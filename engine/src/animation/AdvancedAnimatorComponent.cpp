#include "engine/animation/AdvancedAnimatorComponent.h"
#include "engine/animation/Animator.h"
#include "engine/resources/ModelData.h"
#include "engine/Log.h"

namespace se {
namespace anim {

void AdvancedAnimatorComponent::Initialize(const AdvancedAnimatorConfig& config, 
                                            const SkinnedModelData* skeleton) {
    config_ = config;
    skeleton_ = skeleton;
    
    if (!skeleton_) {
        SE_LOG_ERROR("[AdvancedAnimatorComponent] Cannot initialize without skeleton");
        return;
    }
    
    // Initialize locomotion controller
    locomotion_.Initialize(config_.locomotion, skeleton_);
    
    // Initialize procedural controllers
    lookAt_ = std::make_unique<LookAtController>();
    lookAt_->Initialize(skeleton_, config_.lookAt);
    
    bodyRotation_ = std::make_unique<BodyRotationController>();
    bodyRotation_->Initialize(config_.bodyRotation);
    
    // Initialize poses
    size_t boneCount = skeleton_->Bones.size();
    basePose_.Resize(boneCount);
    finalPose_.Resize(boneCount);
    
    // Initialize IK solvers
    for (auto& solver : ikSolvers_) {
        solver->Initialize(skeleton_);
    }
    
    // Initialize attachments
    for (auto& attachment : attachments_) {
        attachment.Initialize(attachment.GetConfig(), skeleton_);
    }
    
    initialized_ = true;
    SE_LOG_INFO("[AdvancedAnimatorComponent] Initialized with {} bones", boneCount);
}

void AdvancedAnimatorComponent::Update(float dt, Animator* animator) {
    if (!initialized_ || !animator) {
        return;
    }
    
    // Update locomotion (produces base pose)
    UpdateLocomotion(dt);
    
    // Update procedural animation
    UpdateProceduralAnimation(dt);
    
    // Update layer stack
    UpdateLayers();
    
    // Update IK
    UpdateIK();
    
    // Apply final pose to animator
    ApplyFinalPose(animator);
}

void AdvancedAnimatorComponent::SetGraph(std::unique_ptr<AnimationGraph> graph) {
    graph_ = std::move(graph);
    if (graph_ && skeleton_) {
        graph_->SetSkeleton(skeleton_);
    }
}

void AdvancedAnimatorComponent::AddIKSolver(std::unique_ptr<IIKSolver> solver) {
    if (initialized_ && skeleton_) {
        solver->Initialize(skeleton_);
    }
    ikSolvers_.push_back(std::move(solver));
}

IIKSolver* AdvancedAnimatorComponent::GetIKSolver(const std::string& name) {
    for (auto& solver : ikSolvers_) {
        if (solver->GetName() == name) {
            return solver.get();
        }
    }
    return nullptr;
}

size_t AdvancedAnimatorComponent::AddAttachment(const BoneAttachmentConfig& config) {
    BoneAttachment attachment(config);
    if (initialized_ && skeleton_) {
        attachment.Initialize(config, skeleton_);
    }
    attachments_.push_back(std::move(attachment));
    return attachments_.size() - 1;
}

BoneAttachment* AdvancedAnimatorComponent::GetAttachment(size_t index) {
    if (index < attachments_.size()) {
        return &attachments_[index];
    }
    return nullptr;
}

BoneAttachment* AdvancedAnimatorComponent::GetAttachment(const std::string& boneName) {
    for (auto& attachment : attachments_) {
        if (attachment.GetBoneName() == boneName) {
            return &attachment;
        }
    }
    return nullptr;
}

void AdvancedAnimatorComponent::UpdateLocomotion(float dt) {
    if (locomotion_.IsInitialized()) {
        locomotion_.Update(dt, basePose_);
    } else if (graph_) {
        // Use animation graph instead
        graph_->Update(dt);
        graph_->Evaluate(basePose_);
    }
    
    // Start with base pose
    finalPose_ = basePose_;
}

void AdvancedAnimatorComponent::UpdateProceduralAnimation(float dt) {
    // Update look-at controller
    if (lookAt_ && lookAt_->IsEnabled()) {
        lookAt_->Update(dt);
        lookAt_->ApplyToPose(finalPose_);
    }
    
    // Update body rotation
    if (bodyRotation_ && bodyRotation_->IsEnabled()) {
        // Body rotation is typically applied to entity transform, not pose
    }
}

void AdvancedAnimatorComponent::UpdateIK() {
    for (auto& solver : ikSolvers_) {
        if (solver && solver->IsEnabled()) {
            // IK targets should be set by the user before Update is called
            // The solver will apply IK to the final pose
        }
    }
}

void AdvancedAnimatorComponent::UpdateLayers() {
    // Evaluate layer stack on top of final pose
    if (layers_.GetLayerCount() > 0) {
        layers_.SetLayerPose("BaseLocomotion", basePose_);
        layers_.Evaluate(finalPose_);
    }
}

void AdvancedAnimatorComponent::UpdateAttachments(Animator* animator, const glm::mat4& worldMatrix) {
    for (auto& attachment : attachments_) {
        attachment.Update(animator, worldMatrix);
    }
}

void AdvancedAnimatorComponent::ApplyFinalPose(Animator* animator) {
    if (animator && !finalPose_.IsEmpty()) {
        animator->ApplyPose(finalPose_);
    }
}

}  // namespace anim
}  // namespace se
