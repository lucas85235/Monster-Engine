#include "CharacterRender.h"

#include "engine/Application.h"
#include "engine/ecs/AnimatorComponent.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/ecs/SkinnedModelComponent.h"
#include "engine/animation/AnimationManager.h"
#include "engine/animation/SkinnedModelManager.h"

namespace FirstGame {

void CharacterRender::Awake() {
}

void CharacterRender::Start() {
    if (!LoadModel()) {
        return;
    }
    
    ApplyTransformCorrections();
    
    if (modelData_ && modelData_->HasSkeleton()) {
        SetupAnimator();
    }
}

void CharacterRender::Update(float dt) {
}

bool CharacterRender::LoadModel() {
    auto skinnedModel = SkinnedModelManager::Load(config_.ModelPath);
    if (!skinnedModel) {
        SE_LOG_ERROR("CharacterRender: Failed to load model from '{}'", config_.ModelPath);
        return false;
    }
    
    visualEntity_ = GetScene()->CreateEntity("CharacterModel");
    visualEntity_.SetParent(GetEntity());
    
    visualEntity_.AddComponent<SkinnedModelComponent>(skinnedModel);
    modelData_ = skinnedModel->GetModelData();
    
    return true;
}

void CharacterRender::ApplyTransformCorrections() {
    if (!visualEntity_) return;
    
    auto& transform = visualEntity_.GetComponent<TransformComponent>();
    transform.SetScale(config_.Scale);
    transform.SetPosition(config_.Offset);
}

bool CharacterRender::SetupAnimator() {
    if (!visualEntity_ || !modelData_) return false;
    
    auto& animComp = visualEntity_.AddComponent<AnimatorComponent>();
    animComp.Init(modelData_);
    
    // Create animation controller with Idle/Jog states
    animController_ = std::make_shared<AnimatorController>("CharacterAnimator");
    
    // Load clips
    auto idleClip = AnimationManager::Load(config_.IdleAnimPath);
    auto jogClip = AnimationManager::Load(config_.JogAnimPath);
    
    if (!idleClip || !jogClip) {
        SE_LOG_ERROR("CharacterRender: Failed to load animation clips");
        return false;
    }
    
    // Add states
    animController_->AddState({"Idle", idleClip, 1.0f, true});
    animController_->AddState({"Jog", jogClip, 1.0f, true});
    animController_->SetDefaultState("Idle");
    
    // Add parameter
    animController_->AddParameter("IsMoving", false);
    
    // Add transitions: Idle <-> Jog based on IsMoving parameter
    AnimationTransition idleToJog;
    idleToJog.FromState = "Idle";
    idleToJog.ToState = "Jog";
    idleToJog.TransitionDuration = config_.TransitionDuration;
    idleToJog.Conditions.push_back({"IsMoving", TransitionCondition::CompareMode::Equals, true});
    animController_->AddTransition(idleToJog);
    
    AnimationTransition jogToIdle;
    jogToIdle.FromState = "Jog";
    jogToIdle.ToState = "Idle";
    jogToIdle.TransitionDuration = config_.TransitionDuration;
    jogToIdle.Conditions.push_back({"IsMoving", TransitionCondition::CompareMode::Equals, false});
    animController_->AddTransition(jogToIdle);
    
    // Set controller on animator
    animComp.SetController(animController_);
    
    return true;
}

void CharacterRender::SetMoving(bool moving) {
    if (isMoving_ == moving) return;
    
    isMoving_ = moving;
    
    if (animController_) {
        animController_->SetBool("IsMoving", moving);
    }
}

} // namespace FirstGame
