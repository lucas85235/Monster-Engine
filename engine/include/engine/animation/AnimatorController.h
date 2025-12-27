#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "engine/animation/AnimationClip.h"

namespace se {

// Animation parameter types for state machine control
using AnimationParameterValue = std::variant<bool, float, int>;

struct AnimationParameter {
    std::string Name;
    AnimationParameterValue Value;
    
    bool GetBool() const { return std::holds_alternative<bool>(Value) ? std::get<bool>(Value) : false; }
    float GetFloat() const { return std::holds_alternative<float>(Value) ? std::get<float>(Value) : 0.0f; }
    int GetInt() const { return std::holds_alternative<int>(Value) ? std::get<int>(Value) : 0; }
};

// Condition for state transitions
struct TransitionCondition {
    std::string ParameterName;
    enum class CompareMode { Equals, NotEquals, Greater, Less, GreaterEqual, LessEqual };
    CompareMode Mode = CompareMode::Equals;
    AnimationParameterValue Threshold;
    
    bool Evaluate(const std::unordered_map<std::string, AnimationParameter>& params) const;
};

// A single animation state
struct AnimationState {
    std::string Name;
    std::shared_ptr<AnimationClip> Clip;
    float Speed = 1.0f;
    bool Loop = true;
    
    AnimationState() = default;
    AnimationState(const std::string& name, std::shared_ptr<AnimationClip> clip, float speed = 1.0f, bool loop = true)
        : Name(name), Clip(clip), Speed(speed), Loop(loop) {}
};

// Transition between states
struct AnimationTransition {
    std::string FromState;
    std::string ToState;
    std::vector<TransitionCondition> Conditions;
    float TransitionDuration = 0.25f;
    bool HasExitTime = false;
    float ExitTime = 1.0f;
    
    bool CanTransition(const std::unordered_map<std::string, AnimationParameter>& params, 
                       float normalizedTime) const;
};

// Controller that manages animation states and transitions
class AnimatorController {
public:
    AnimatorController() = default;
    explicit AnimatorController(const std::string& name);
    
    // State management
    void AddState(const AnimationState& state);
    void RemoveState(const std::string& name);
    AnimationState* GetState(const std::string& name);
    const AnimationState* GetState(const std::string& name) const;
    void SetDefaultState(const std::string& name) { defaultStateName_ = name; }
    const std::string& GetDefaultStateName() const { return defaultStateName_; }
    
    // Transition management
    void AddTransition(const AnimationTransition& transition);
    
    // Parameter management  
    void AddParameter(const std::string& name, bool value);
    void AddParameter(const std::string& name, float value);
    void AddParameter(const std::string& name, int value);
    void SetBool(const std::string& name, bool value);
    void SetFloat(const std::string& name, float value);
    void SetInt(const std::string& name, int value);
    void SetTrigger(const std::string& name);
    void ResetTrigger(const std::string& name);
    
    // Evaluation
    const AnimationTransition* FindTransition(const std::string& fromState, float normalizedTime) const;
    
    const std::string& GetName() const { return name_; }
    const std::unordered_map<std::string, AnimationState>& GetStates() const { return states_; }
    const std::unordered_map<std::string, AnimationParameter>& GetParameters() const { return parameters_; }
    
private:
    std::string name_;
    std::string defaultStateName_;
    std::unordered_map<std::string, AnimationState> states_;
    std::unordered_map<std::string, AnimationParameter> parameters_;
    std::vector<AnimationTransition> transitions_;
};

}  // namespace se
