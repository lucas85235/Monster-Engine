#include "engine/animation/AnimatorController.h"

#include "engine/Log.h"

namespace se {

bool TransitionCondition::Evaluate(const std::unordered_map<std::string, AnimationParameter>& params) const {
    auto it = params.find(ParameterName);
    if (it == params.end()) return false;
    
    const auto& param = it->second;
    
    // Bool comparison
    if (std::holds_alternative<bool>(Threshold) && std::holds_alternative<bool>(param.Value)) {
        bool threshold = std::get<bool>(Threshold);
        bool value = std::get<bool>(param.Value);
        switch (Mode) {
            case CompareMode::Equals: return value == threshold;
            case CompareMode::NotEquals: return value != threshold;
            default: return false;
        }
    }
    
    // Float comparison
    if (std::holds_alternative<float>(Threshold) && std::holds_alternative<float>(param.Value)) {
        float threshold = std::get<float>(Threshold);
        float value = std::get<float>(param.Value);
        switch (Mode) {
            case CompareMode::Equals: return value == threshold;
            case CompareMode::NotEquals: return value != threshold;
            case CompareMode::Greater: return value > threshold;
            case CompareMode::Less: return value < threshold;
            case CompareMode::GreaterEqual: return value >= threshold;
            case CompareMode::LessEqual: return value <= threshold;
        }
    }
    
    // Int comparison
    if (std::holds_alternative<int>(Threshold) && std::holds_alternative<int>(param.Value)) {
        int threshold = std::get<int>(Threshold);
        int value = std::get<int>(param.Value);
        switch (Mode) {
            case CompareMode::Equals: return value == threshold;
            case CompareMode::NotEquals: return value != threshold;
            case CompareMode::Greater: return value > threshold;
            case CompareMode::Less: return value < threshold;
            case CompareMode::GreaterEqual: return value >= threshold;
            case CompareMode::LessEqual: return value <= threshold;
        }
    }
    
    return false;
}

bool AnimationTransition::CanTransition(const std::unordered_map<std::string, AnimationParameter>& params,
                                         float normalizedTime) const {
    // Check exit time condition
    if (HasExitTime && normalizedTime < ExitTime) {
        return false;
    }
    
    // All conditions must pass
    for (const auto& condition : Conditions) {
        if (!condition.Evaluate(params)) {
            return false;
        }
    }
    
    return true;
}

AnimatorController::AnimatorController(const std::string& name) : name_(name) {
    SE_LOG_INFO("AnimatorController '{}' created", name_);
}

void AnimatorController::AddState(const AnimationState& state) {
    if (states_.empty()) {
        defaultStateName_ = state.Name;
    }
    states_[state.Name] = state;
    SE_LOG_INFO("AnimatorController '{}': Added state '{}'", name_, state.Name);
}

void AnimatorController::RemoveState(const std::string& name) {
    states_.erase(name);
}

AnimationState* AnimatorController::GetState(const std::string& name) {
    auto it = states_.find(name);
    return it != states_.end() ? &it->second : nullptr;
}

const AnimationState* AnimatorController::GetState(const std::string& name) const {
    auto it = states_.find(name);
    return it != states_.end() ? &it->second : nullptr;
}

void AnimatorController::AddTransition(const AnimationTransition& transition) {
    transitions_.push_back(transition);
    SE_LOG_INFO("AnimatorController '{}': Added transition from '{}' to '{}'", 
                name_, transition.FromState, transition.ToState);
}

void AnimatorController::AddParameter(const std::string& name, bool value) {
    parameters_[name] = AnimationParameter{name, value};
}

void AnimatorController::AddParameter(const std::string& name, float value) {
    parameters_[name] = AnimationParameter{name, value};
}

void AnimatorController::AddParameter(const std::string& name, int value) {
    parameters_[name] = AnimationParameter{name, value};
}

void AnimatorController::SetBool(const std::string& name, bool value) {
    auto it = parameters_.find(name);
    if (it != parameters_.end() && std::holds_alternative<bool>(it->second.Value)) {
        it->second.Value = value;
    }
}

void AnimatorController::SetFloat(const std::string& name, float value) {
    auto it = parameters_.find(name);
    if (it != parameters_.end() && std::holds_alternative<float>(it->second.Value)) {
        it->second.Value = value;
    }
}

void AnimatorController::SetInt(const std::string& name, int value) {
    auto it = parameters_.find(name);
    if (it != parameters_.end() && std::holds_alternative<int>(it->second.Value)) {
        it->second.Value = value;
    }
}

void AnimatorController::SetTrigger(const std::string& name) {
    SetBool(name, true);
}

void AnimatorController::ResetTrigger(const std::string& name) {
    SetBool(name, false);
}

const AnimationTransition* AnimatorController::FindTransition(const std::string& fromState, 
                                                               float normalizedTime) const {
    for (const auto& transition : transitions_) {
        if (transition.FromState == fromState && transition.CanTransition(parameters_, normalizedTime)) {
            return &transition;
        }
    }
    return nullptr;
}

}  // namespace se
