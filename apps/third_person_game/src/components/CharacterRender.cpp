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
    // Animation is managed by engine's AnimationSystem
}

bool CharacterRender::LoadModel() {
    auto skinnedModel = SkinnedModelManager::Load(config_.ModelPath);
    if (!skinnedModel) {
        SE_LOG_ERROR("CharacterRender: Failed to load model from '{}'", config_.ModelPath);
        return false;
    }
    
    // Create child entity for visual (separate from physics collider)
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
    
    if (config_.AutoPlayAnimation && !config_.DefaultAnimationPath.empty()) {
        PlayAnimation(config_.DefaultAnimationPath, config_.LoopAnimation);
    }
    
    return true;
}

void CharacterRender::PlayAnimation(const std::string& animPath, bool loop) {
    if (!visualEntity_ || !visualEntity_.HasComponent<AnimatorComponent>()) {
        SE_LOG_WARN("CharacterRender::PlayAnimation: No AnimatorComponent");
        return;
    }
    
    auto clip = AnimationManager::Load(animPath);
    if (!clip) {
        SE_LOG_ERROR("CharacterRender: Failed to load animation '{}'", animPath);
        return;
    }
    
    visualEntity_.GetComponent<AnimatorComponent>().Play(clip, loop);
}

void CharacterRender::StopAnimation() {
    if (visualEntity_ && visualEntity_.HasComponent<AnimatorComponent>()) {
        visualEntity_.GetComponent<AnimatorComponent>().Stop();
    }
}

bool CharacterRender::IsAnimating() const {
    if (visualEntity_ && visualEntity_.HasComponent<AnimatorComponent>()) {
        return visualEntity_.GetComponent<AnimatorComponent>().playing;
    }
    return false;
}

} // namespace FirstGame
