#pragma once

#include "engine/ai/Blackboard.h"
#include "engine/ai/StateNode.h"
#include "engine/Log.h"

#include <memory>
#include <vector>
#include <unordered_map>

namespace se {

class AIController;

class StateTree {
   public:
    StateTree() = default;
    ~StateTree() = default;

    // Add a state to the tree (takes ownership)
    template <typename T, typename... Args>
    T* AddState(const std::string& name, Args&&... args) {
        auto state = std::make_unique<T>(std::forward<Args>(args)...);
        state->SetName(name);
        T* ptr = state.get();
        states_[name] = std::move(state);
        return ptr;
    }

    // Set initial state by pointer or name
    void SetInitialState(StateNode* state) { initialState_ = state; }
    void SetInitialState(const std::string& name) {
        auto it = states_.find(name);
        if (it != states_.end()) {
            initialState_ = static_cast<StateNode*>(it->second.get());
        }
    }

    // Get state by name
    StateNode* GetState(const std::string& name) {
        auto it = states_.find(name);
        return it != states_.end() ? static_cast<StateNode*>(it->second.get()) : nullptr;
    }

    // Start the state tree
    void Start(AIController* controller);

    // Update the state tree
    void Tick(AIController* controller, float dt);

    // Stop the state tree
    void Stop();

    // Current state info
    StateNode*         GetCurrentState() const { return currentState_; }
    const std::string& GetCurrentStateName() const;
    float              GetTimeInCurrentState() const { return stateTime_; }

    // Blackboard access
    Blackboard& GetBlackboard() { return blackboard_; }
    const Blackboard& GetBlackboard() const { return blackboard_; }

    // Force transition to a state
    void TransitionTo(StateNode* state);
    void TransitionTo(const std::string& stateName);

   private:
    void PerformTransition(StateNode* newState, StateTreeContext& ctx);

    std::unordered_map<std::string, std::unique_ptr<StateTreeNode>> states_;
    StateNode*  currentState_  = nullptr;
    StateNode*  initialState_  = nullptr;
    Blackboard  blackboard_;
    float       stateTime_     = 0.0f;
    bool        isStarted_     = false;
};

}  // namespace se
