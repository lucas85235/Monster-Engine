#include "engine/ai/StateTree.h"
#include "engine/gameplay/AIController.h"
#include "engine/Log.h"

namespace se {

static const std::string kEmptyString;

void StateTree::Start(AIController* controller) {
    if (isStarted_) return;

    if (!initialState_) {
        SE_LOG_WARN("[StateTree] No initial state set");
        return;
    }

    StateTreeContext ctx;
    ctx.blackboard = &blackboard_;
    ctx.controller = controller;
    ctx.deltaTime  = 0.0f;
    ctx.stateTime  = 0.0f;

    currentState_ = initialState_;
    stateTime_    = 0.0f;
    currentState_->Enter(ctx);
    isStarted_ = true;

    SE_LOG_DEBUG("[StateTree] Started with state '{}'", currentState_->GetName());
}

void StateTree::Tick(AIController* controller, float dt) {
    if (!isStarted_ || !currentState_) return;

    stateTime_ += dt;

    StateTreeContext ctx;
    ctx.blackboard = &blackboard_;
    ctx.controller = controller;
    ctx.deltaTime  = dt;
    ctx.stateTime  = stateTime_;

    // Check transitions first
    StateNode* nextState = currentState_->EvaluateTransitions(ctx);
    if (nextState && nextState != currentState_) {
        PerformTransition(nextState, ctx);
    }

    // Tick current state
    currentState_->Tick(ctx);
}

void StateTree::Stop() {
    if (!isStarted_) return;

    if (currentState_) {
        StateTreeContext ctx;
        ctx.blackboard = &blackboard_;
        ctx.controller = nullptr;
        ctx.deltaTime  = 0.0f;
        ctx.stateTime  = stateTime_;
        currentState_->Exit(ctx);
    }

    currentState_ = nullptr;
    isStarted_    = false;
    stateTime_    = 0.0f;

    SE_LOG_DEBUG("[StateTree] Stopped");
}

const std::string& StateTree::GetCurrentStateName() const {
    return currentState_ ? currentState_->GetName() : kEmptyString;
}

void StateTree::TransitionTo(StateNode* state) {
    if (!state || state == currentState_) return;

    StateTreeContext ctx;
    ctx.blackboard = &blackboard_;
    ctx.controller = nullptr;
    ctx.deltaTime  = 0.0f;
    ctx.stateTime  = stateTime_;

    PerformTransition(state, ctx);
}

void StateTree::TransitionTo(const std::string& stateName) {
    TransitionTo(GetState(stateName));
}

void StateTree::PerformTransition(StateNode* newState, StateTreeContext& ctx) {
    if (!newState) return;

    std::string oldName = currentState_ ? currentState_->GetName() : "none";

    if (currentState_) {
        currentState_->Exit(ctx);
    }

    currentState_ = newState;
    stateTime_    = 0.0f;
    ctx.stateTime = 0.0f;

    currentState_->Enter(ctx);

    SE_LOG_DEBUG("[StateTree] Transition: '{}' -> '{}'", oldName, currentState_->GetName());
}

}  // namespace se
