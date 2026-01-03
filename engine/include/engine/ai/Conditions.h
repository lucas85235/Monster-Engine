#pragma once

#include "engine/ai/StateNode.h"

namespace se {
namespace ai {

// Condition builders for common scenarios
namespace Conditions {

// Check if distance to target is within range
inline ConditionFunc DistanceLessThan(float distance) {
    return [distance](const StateTreeContext& ctx) {
        return ctx.blackboard->GetDistanceToTarget() < distance;
    };
}

inline ConditionFunc DistanceGreaterThan(float distance) {
    return [distance](const StateTreeContext& ctx) {
        return ctx.blackboard->GetDistanceToTarget() > distance;
    };
}

// Check if can see target
inline ConditionFunc CanSeeTarget() {
    return [](const StateTreeContext& ctx) {
        return ctx.blackboard->GetCanSeeTarget();
    };
}

inline ConditionFunc CannotSeeTarget() {
    return [](const StateTreeContext& ctx) {
        return !ctx.blackboard->GetCanSeeTarget();
    };
}

// Check if has target
inline ConditionFunc HasTarget() {
    return [](const StateTreeContext& ctx) {
        return ctx.blackboard->Has("Target");
    };
}

// Timer condition - true after duration in current state
inline ConditionFunc AfterSeconds(float duration) {
    return [duration](const StateTreeContext& ctx) {
        return ctx.stateTime >= duration;
    };
}

// Blackboard value conditions
template <typename T>
inline ConditionFunc BlackboardEquals(const std::string& key, const T& value) {
    return [key, value](const StateTreeContext& ctx) {
        return ctx.blackboard->Get<T>(key) == value;
    };
}

inline ConditionFunc BlackboardHas(const std::string& key) {
    return [key](const StateTreeContext& ctx) {
        return ctx.blackboard->Has(key);
    };
}

// Combine conditions (AND)
inline ConditionFunc All(std::initializer_list<ConditionFunc> conditions) {
    std::vector<ConditionFunc> conds(conditions);
    return [conds](const StateTreeContext& ctx) {
        for (const auto& c : conds) {
            if (!c(ctx)) return false;
        }
        return true;
    };
}

// Combine conditions (OR)
inline ConditionFunc Any(std::initializer_list<ConditionFunc> conditions) {
    std::vector<ConditionFunc> conds(conditions);
    return [conds](const StateTreeContext& ctx) {
        for (const auto& c : conds) {
            if (c(ctx)) return true;
        }
        return false;
    };
}

// Negate condition
inline ConditionFunc Not(ConditionFunc condition) {
    return [condition](const StateTreeContext& ctx) {
        return !condition(ctx);
    };
}

}  // namespace Conditions
}  // namespace ai
}  // namespace se
