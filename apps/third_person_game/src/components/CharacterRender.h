#pragma once
/**
 * CharacterRender - Visual representation of a character with skeletal animation.
 *
 * Architecture:
 * - SkinnedModelComponent: Holds mesh with bone data
 * - AnimatorComponent: Managed by engine's AnimationSystem
 * - RenderSystem: Reads bone matrices and renders
 */

#include "engine/ecs/Component.h"
#include "engine/ecs/Entity.h"
#include "engine/animation/AnimationClip.h"
#include "engine/resources/ModelData.h"

#include <glm.hpp>
#include <memory>
#include <string>

namespace FirstGame {
using namespace se;

// Configuration for character visual (Open/Closed principle - extend via config)
struct CharacterRenderConfig {
    // Model settings
    std::string ModelPath = "assets/models/characters/Y_Bot.fbx";
    std::string DefaultAnimationPath = "assets/models/characters/animations/YBot_JogForward.fbx";
    
    // Transform corrections for imported models
    glm::vec3 Scale{0.01f};
    glm::vec3 Offset{0.0f, -0.85f, 0.0f};
    
    // Animation settings
    bool AutoPlayAnimation = true;
    bool LoopAnimation = true;
};

// Animation control interface (Interface Segregation)
class IAnimationController {
public:
    virtual ~IAnimationController() = default;
    virtual void PlayAnimation(const std::string& path, bool loop = true) = 0;
    virtual void StopAnimation() = 0;
    virtual bool IsAnimating() const = 0;
};

class CharacterRender : public Component, public IAnimationController {
public:
    CharacterRender() = default;
    explicit CharacterRender(const CharacterRenderConfig& config) : config_(config) {}
    ~CharacterRender() override = default;

    // Component lifecycle
    void Awake() override;
    void Start() override;
    void Update(float dt) override;

    // Configuration (call before Start for effect)
    void SetConfig(const CharacterRenderConfig& config) { config_ = config; }
    CharacterRenderConfig& GetConfig() { return config_; }
    const CharacterRenderConfig& GetConfig() const { return config_; }
    
    // IAnimationController implementation
    void PlayAnimation(const std::string& animPath, bool loop = true) override;
    void StopAnimation() override;
    bool IsAnimating() const override;
    
    // Access to visual entity (for advanced usage)
    Entity GetVisualEntity() const { return visualEntity_; }

private:
    // SRP: Each method does one thing
    bool LoadModel();
    bool SetupAnimator();
    void ApplyTransformCorrections();

    CharacterRenderConfig config_;
    Entity visualEntity_;
    std::shared_ptr<SkinnedModelData> modelData_;
};

} // namespace FirstGame