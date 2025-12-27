#include "CharacterRender.h"

#include "apps/SampleUtilities.h"
#include "engine/Application.h"
#include "engine/ecs/AnimatorComponent.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/ecs/SkinnedModelComponent.h"
#include "engine/physics/PhysicsDebugDraw.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/animation/AnimationManager.h"
#include "engine/animation/SkinnedModelManager.h"
#include "engine/animation/SkinnedModelLoader.h"
#include "LinearMath/btIDebugDraw.h"

namespace FirstGame {

void CharacterRender::Awake() {
    SetupDebugVisualization();
    SE_LOG_INFO("CharacterRender::Awake() - Initialized");
}

void CharacterRender::Start() {
    SetupMesh();
    SE_LOG_INFO("CharacterRender::Start() - Model and animation loaded");
}

void CharacterRender::Update(float dt) {
    // Update AnimatorComponent every frame
    if (visualEntity_ && visualEntity_.HasComponent<AnimatorComponent>()) {
        auto& animComp = visualEntity_.GetComponent<AnimatorComponent>();
        animComp.Update(dt);
        
        // Debug log animation status occasionally
        static float logTimer = 0.0f;
        logTimer += dt;
        if (logTimer > 2.0f) {
            logTimer = 0.0f;
            const auto& bones = animComp.GetBoneMatrices();
            SE_LOG_INFO("CharacterRender: Animation playing={}, bones={}", 
                        animComp.playing, bones.size());
        }
    }
}

void CharacterRender::SetupMesh() {
    const std::string modelPath = "assets/models/characters/Y_Bot.fbx";
    
    // Load SkinnedModel (with bone data in vertices)
    auto skinnedModel = SkinnedModelManager::Load(modelPath);
    if (!skinnedModel) {
        SE_LOG_ERROR("CharacterRender: Failed to load skinned model from '{}'", modelPath);
        return;
    }
    
    // Create child entity for the visual model
    visualEntity_ = GetScene()->CreateEntity("CharacterModel");
    visualEntity_.SetParent(GetEntity());
    
    // Add SkinnedModelComponent for rendering (uses skinned shader with bone attributes)
    visualEntity_.AddComponent<SkinnedModelComponent>(skinnedModel);
    
    // Apply transform corrections for FBX model
    auto& transform = visualEntity_.GetComponent<TransformComponent>();
    transform.SetScale(glm::vec3(0.01f));
    transform.SetPosition(glm::vec3(0.0f, -0.85f, 0.0f));
    
    SE_LOG_INFO("CharacterRender: Loaded SkinnedModel '{}' with {} meshes",
                skinnedModel->GetName(), skinnedModel->GetMeshCount());
    
    // Get model data for animator
    modelData_ = skinnedModel->GetModelData();
    
    if (modelData_ && modelData_->HasSkeleton()) {
        // Add AnimatorComponent to the SAME entity as SkinnedModelComponent
        auto& animComp = visualEntity_.AddComponent<AnimatorComponent>();
        animComp.Init(modelData_);
        
        SE_LOG_INFO("CharacterRender: Added AnimatorComponent with {} bones", modelData_->Bones.size());
        
        // Load and play animation
        auto clip = AnimationManager::Load("assets/models/characters/animations/YBot_JogForward.fbx");
        if (clip) {
            animComp.Play(clip, true);
            currentAnimation_ = clip;
            SE_LOG_INFO("CharacterRender: Playing animation '{}'", clip->GetName());
        } else {
            SE_LOG_WARN("CharacterRender: Failed to load animation");
        }
    } else {
        SE_LOG_WARN("CharacterRender: Model has no skeleton data for animation");
    }
}

void CharacterRender::PlayAnimation(const std::string& animPath, bool loop) {
    if (!visualEntity_ || !visualEntity_.HasComponent<AnimatorComponent>()) {
        SE_LOG_WARN("CharacterRender::PlayAnimation: No AnimatorComponent available");
        return;
    }
    
    auto& animComp = visualEntity_.GetComponent<AnimatorComponent>();
    
    currentAnimation_ = AnimationManager::Load(animPath);
    if (!currentAnimation_) {
        SE_LOG_ERROR("CharacterRender: Failed to load animation '{}'", animPath);
        return;
    }
    
    animComp.Play(currentAnimation_, loop);
    SE_LOG_INFO("CharacterRender: Playing animation '{}' (loop: {})", 
                currentAnimation_->GetName(), loop);
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

void CharacterRender::SetupDebugVisualization() {
    Scene* scene = GetScene();
    if (!scene || !scene->GetPhysicsSystem()) {
        SE_LOG_WARN("CharacterRender: Cannot configure debug - no physics system");
        return;
    }

    auto* debugDrawer = scene->GetPhysicsSystem()->GetDebugDrawer();
    if (debugDrawer) {
        if (config_.enablePhysicsDebug) {
            debugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
            SE_LOG_INFO("CharacterRender: Physics debug enabled");
        } else {
            debugDrawer->setDebugMode(btIDebugDraw::DBG_NoDebug);
            SE_LOG_INFO("CharacterRender: Physics debug disabled");
        }
    }
}

} // namespace FirstGame