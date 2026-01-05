#pragma once
/**
 * AnimatorAsset.h - Serializable animator configuration data.
 * 
 * Contains all data needed to save/load animator configurations,
 * including states, transitions, blend spaces, layers, and procedural settings.
 */

#include "engine/animation/advanced/BlendSpace.h"
#include "engine/animation/advanced/AnimationLayer.h"
#include "engine/animation/advanced/LookAtController.h"
#include "engine/animation/advanced/BodyRotationController.h"

#include <string>
#include <vector>
#include <variant>
#include <cstdint>

namespace se {
namespace anim {

static constexpr uint32_t ANIMATOR_ASSET_MAGIC = 0x41534D54;  // "ASMT"
static constexpr uint32_t ANIMATOR_ASSET_VERSION = 1;

enum class ParameterType {
    Bool,
    Float,
    Int,
    Trigger
};

struct AnimationParameterData {
    std::string name;
    ParameterType type = ParameterType::Bool;
    std::variant<bool, float, int> defaultValue;
    
    AnimationParameterData() : defaultValue(false) {}
    AnimationParameterData(const std::string& n, bool val) : name(n), type(ParameterType::Bool), defaultValue(val) {}
    AnimationParameterData(const std::string& n, float val) : name(n), type(ParameterType::Float), defaultValue(val) {}
    AnimationParameterData(const std::string& n, int val) : name(n), type(ParameterType::Int), defaultValue(val) {}
};

struct TransitionConditionData {
    std::string parameterName;
    std::string compareMode;  // "Equals", "NotEquals", "Greater", "Less", "GreaterEqual", "LessEqual"
    std::variant<bool, float, int> threshold;
    
    TransitionConditionData() : threshold(false) {}
};

struct AnimationStateData {
    std::string name;
    std::string clipPath;
    std::string blendSpaceName;
    float speed = 1.0f;
    bool loop = true;
    bool isBlendSpace = false;
};

struct AnimationTransitionData {
    std::string fromState;
    std::string toState;
    std::vector<TransitionConditionData> conditions;
    float duration = 0.25f;
    bool hasExitTime = false;
    float exitTime = 1.0f;
};

struct AnimatorAssetData {
    std::string name;
    std::string skeletonPath;
    
    // State machine
    std::vector<AnimationStateData> states;
    std::vector<AnimationTransitionData> transitions;
    std::string defaultStateName;
    std::vector<AnimationParameterData> parameters;
    
    // Blend spaces
    std::vector<BlendSpaceData> blendSpaces1D;
    std::vector<BlendSpaceData> blendSpaces2D;
    
    // Layers
    std::vector<AnimationLayerData> layers;
    std::vector<BoneMaskData> boneMasks;
    
    // Procedural controllers
    LookAtSettings lookAtSettings;
    BodyRotationSettings bodyRotationSettings;
    
    void Clear() {
        name.clear();
        skeletonPath.clear();
        states.clear();
        transitions.clear();
        defaultStateName.clear();
        parameters.clear();
        blendSpaces1D.clear();
        blendSpaces2D.clear();
        layers.clear();
        boneMasks.clear();
        lookAtSettings = LookAtSettings();
        bodyRotationSettings = BodyRotationSettings();
    }
};

}  // namespace anim
}  // namespace se
