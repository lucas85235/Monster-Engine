#pragma once

#include <string>

namespace se {

class Blackboard;
class AIController;

// Result status of node execution
enum class NodeStatus {
    Running,  // Still executing
    Success,  // Completed successfully
    Failure   // Failed
};

// Context passed to nodes during execution
struct StateTreeContext {
    Blackboard*   blackboard  = nullptr;
    AIController* controller  = nullptr;
    float         deltaTime   = 0.0f;
    float         stateTime   = 0.0f;  // Time in current state
};

// Base class for all State Tree nodes
class StateTreeNode {
   public:
    StateTreeNode() = default;
    virtual ~StateTreeNode() = default;

    // Lifecycle methods
    virtual void Enter(StateTreeContext& ctx) {}
    virtual NodeStatus Tick(StateTreeContext& ctx) = 0;
    virtual void Exit(StateTreeContext& ctx) {}

    // Node identification
    void SetName(const std::string& name) { name_ = name; }
    const std::string& GetName() const { return name_; }

   protected:
    std::string name_;
};

}  // namespace se
