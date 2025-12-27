#pragma once
/**
 * CharacterRender.h - Character visual representation with skeletal animation.
 *
 * This component manages:
 * - Skinned mesh rendering (via SkinnedModelComponent on child entity)
 * - Skeletal animation playback (via AnimatorComponent on same child entity)
 * - Debug visualization
 *
 * Architecture (Unity-style):
 * - SkinnedModelComponent holds the SkinnedModel (mesh with bone data in vertices)
 * - AnimatorComponent holds the Animator and AnimationClip
 * - RenderSystem detects both and sends bone matrices to skinned shader
 *
 * Requires: TransformComponent
 */

#include "engine/ecs/Component.h"
#include "engine/ecs/Entity.h"
#include "engine/animation/AnimationClip.h"
#include "engine/resources/ModelData.h"

#include <memory>

namespace FirstGame {
using namespace se;

struct RenderConfig {
    bool enablePhysicsDebug = false;
    bool showBoneDebug = false;
};

class CharacterRender : public Component {
public:
    CharacterRender() = default;
    ~CharacterRender() override = default;

    void Awake() override;
    void Start() override;
    void Update(float dt) override;

    RenderConfig& GetConfig() { return config_; }
    
    // Animation control
    void PlayAnimation(const std::string& animPath, bool loop = true);
    void StopAnimation();
    bool IsAnimating() const;

private:
    void SetupMesh();
    void SetupDebugVisualization();

    RenderConfig config_;
    
    // Visual entity containing SkinnedModelComponent + AnimatorComponent
    Entity visualEntity_;
    
    // Animation data
    std::shared_ptr<SkinnedModelData> modelData_;
    std::shared_ptr<AnimationClip> currentAnimation_;
};

} // namespace FirstGame