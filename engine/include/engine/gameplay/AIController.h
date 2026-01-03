#pragma once

#include "engine/gameplay/Controller.h"
#include "engine/navigation/PathfindingAgentComponent.h"
#include "engine/ai/StateTree.h"

#include <memory>

namespace se {

namespace nav {
class NavigationSystem;
}

class AIController : public Controller {
   public:
    AIController()           = default;
    ~AIController() override = default;

    void Start() override;
    void Update(float dt) override;

    bool IsAIController() const override { return true; }

    // State Tree
    void SetStateTree(std::unique_ptr<StateTree> tree);
    StateTree* GetStateTree() { return stateTree_.get(); }
    Blackboard& GetBlackboard();

    // Navigation system (must be set before using movement commands)
    void SetNavigationSystem(nav::NavigationSystem* navSystem);
    nav::NavigationSystem* GetNavigationSystem() const { return navSystem_; }

    // Movement commands
    void MoveToLocation(const Vector3& destination);
    void MoveToEntity(Entity target);
    void StopMovement();

    // State queries
    bool  IsMoving() const;
    bool  HasReachedDestination() const;
    bool  IsPathPending() const;
    float GetRemainingDistance() const;

    // Agent configuration access
    nav::PathfindingAgentComponent& GetPathfindingAgent() { return agent_; }
    const nav::PathfindingAgentComponent& GetPathfindingAgent() const { return agent_; }

    // Events (override in subclasses for custom behavior)
    virtual void OnMoveCompleted(bool success);
    virtual void OnDestinationReached();

   protected:
    void OnPossess(Pawn* pawn) override;
    void OnUnpossess() override;

   private:
    void UpdateBlackboard();
    void UpdatePathfinding(float dt);
    void ApplyMovementToPawn();

    nav::NavigationSystem*         navSystem_ = nullptr;
    nav::PathfindingAgentComponent agent_;
    std::unique_ptr<StateTree>     stateTree_;
    Blackboard                     defaultBlackboard_;
    bool                           hasDestination_ = false;
};

}  // namespace se
