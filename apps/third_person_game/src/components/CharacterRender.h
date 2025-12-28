#pragma once
/**
 * CharacterRender - Visual representation with skeletal animation and state machine.
 */

#include "engine/ecs/Component.h"
#include "engine/ecs/Entity.h"
#include "engine/animation/AnimationClip.h"
#include "engine/animation/AnimatorController.h"
#include "engine/resources/ModelData.h"

#include <glm.hpp>
#include <memory>
#include <string>

namespace FirstGame {
using namespace se;

struct CharacterRenderConfig {
    std::string ModelPath = "assets/models/characters/Y_Bot.fbx";
    std::string IdleAnimPath = "assets/models/characters/animations/YBot_Idle.fbx";
    std::string JogAnimPath = "assets/models/characters/animations/YBot_JogForward.fbx";
    
    glm::vec3 Scale{0.01f};
    // glm::vec3 Scale{1.0f};
    // glm::vec3 Rotation{90.0f, 90.0f, 0.0f};
    glm::vec3 Rotation{0.0f};
    glm::vec3 Offset{0.0f, -0.85f, 0.0f};
    
    float TransitionDuration = 0.2f;
};

class CharacterRender : public Component {
public:
    CharacterRender() = default;
    explicit CharacterRender(const CharacterRenderConfig& config) : config_(config) {}
    ~CharacterRender() override = default;

    void Awake() override;
    void Start() override;
    void Update(float dt) override;

    void SetConfig(const CharacterRenderConfig& config) { config_ = config; }
    CharacterRenderConfig& GetConfig() { return config_; }
    
    // State query
    bool IsMoving() const { return isMoving_; }
    
    Entity GetVisualEntity() const { return visualEntity_; }

private:
    bool LoadModel();
    bool SetupAnimator();
    void ApplyTransformCorrections();
    void UpdateMovementState(bool moving);
    void SetupBoneAttachmentTest();

    CharacterRenderConfig config_;
    Entity visualEntity_;
    std::shared_ptr<SkinnedModelData> modelData_;
    std::shared_ptr<AnimatorController> animController_;
    bool isMoving_ = false;
};

} // namespace FirstGame