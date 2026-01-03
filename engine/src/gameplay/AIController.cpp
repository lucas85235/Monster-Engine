#include "engine/gameplay/AIController.h"
#include "engine/gameplay/Pawn.h"
#include "engine/gameplay/Character.h"
#include "engine/navigation/NavigationSystem.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/Log.h"

namespace se {

void AIController::Start() {
    Controller::Start();
    
    // Setup agent callbacks
    agent_.onArrived = [this]() {
        OnDestinationReached();
    };
    
    agent_.onPathFailed = [this]() {
        OnMoveCompleted(false);
    };

    // Start state tree if set
    if (stateTree_) {
        stateTree_->Start(this);
    }
}

void AIController::Update(float dt) {
    if (!pawn_ || !navSystem_) return;
    
    // Update blackboard with world info
    UpdateBlackboard();
    
    // Run state tree if present
    if (stateTree_) {
        stateTree_->Tick(this, dt);
    }
    
    // Update pathfinding
    UpdatePathfinding(dt);
    
    // Apply movement to pawn
    ApplyMovementToPawn();
}

void AIController::SetStateTree(std::unique_ptr<StateTree> tree) {
    stateTree_ = std::move(tree);
    
    if (stateTree_ && pawn_) {
        stateTree_->Start(this);
    }
}

Blackboard& AIController::GetBlackboard() {
    if (stateTree_) {
        return stateTree_->GetBlackboard();
    }
    return defaultBlackboard_;
}

void AIController::SetNavigationSystem(nav::NavigationSystem* navSystem) {
    navSystem_ = navSystem;
}

void AIController::MoveToLocation(const Vector3& destination) {
    if (!pawn_) {
        SE_LOG_WARN("[AIController] Cannot move: no pawn possessed");
        return;
    }
    
    agent_.SetTarget(destination);
    hasDestination_ = true;
}

void AIController::MoveToEntity(Entity target) {
    if (!pawn_) {
        SE_LOG_WARN("[AIController] Cannot move: no pawn possessed");
        return;
    }
    
    if (!target.IsValid()) {
        SE_LOG_WARN("[AIController] Cannot move: invalid target entity");
        return;
    }
    
    agent_.SetTarget(target);
    hasDestination_ = true;
}

void AIController::StopMovement() {
    agent_.ClearTarget();
    hasDestination_ = false;
    
    if (auto* character = GetPawn<Character>()) {
        character->SetVelocity(Vector3{0.0f, character->GetVelocity().y, 0.0f});
    }
}

bool AIController::IsMoving() const {
    return agent_.IsMoving();
}

bool AIController::HasReachedDestination() const {
    return agent_.HasArrived();
}

bool AIController::IsPathPending() const {
    return agent_.state == nav::AgentState::RequestingPath;
}

float AIController::GetRemainingDistance() const {
    return agent_.GetRemainingDistance();
}

void AIController::OnMoveCompleted(bool success) {
    hasDestination_ = false;
}

void AIController::OnDestinationReached() {
    OnMoveCompleted(true);
}

void AIController::OnPossess(Pawn* pawn) {
    SE_LOG_DEBUG("[AIController] Possessed pawn {}", pawn->GetEntityID());
    
    // Store home position
    if (pawn->GetEntity().HasComponent<TransformComponent>()) {
        Vector3 pos = pawn->GetEntity().GetComponent<TransformComponent>().Position;
        GetBlackboard().Set(BlackboardKeys::HomePosition, pos);
    }
}

void AIController::OnUnpossess() {
    StopMovement();
    if (stateTree_) {
        stateTree_->Stop();
    }
}

void AIController::UpdateBlackboard() {
    if (!pawn_) return;
    
    Entity pawnEntity = pawn_->GetEntity();
    if (!pawnEntity.HasComponent<TransformComponent>()) return;
    
    auto& bb = GetBlackboard();
    
    // Update self position
    Vector3 selfPos = pawnEntity.GetComponent<TransformComponent>().Position;
    bb.SetSelfPosition(selfPos);
    
    // Update distance to target if we have one
    Entity target = bb.GetTarget();
    if (target.IsValid() && target.HasComponent<TransformComponent>()) {
        Vector3 targetPos = target.GetComponent<TransformComponent>().Position;
        bb.SetTargetPosition(targetPos);
        
        float distance = glm::distance(selfPos, targetPos);
        bb.SetDistanceToTarget(distance);
        
        // TODO: Add line of sight check using physics raycast
        bb.SetCanSeeTarget(distance < 20.0f);  // Simplified for now
    }
}

void AIController::UpdatePathfinding(float dt) {
    if (!navSystem_ || !pawn_) return;
    
    Entity pawnEntity = pawn_->GetEntity();
    if (!pawnEntity.HasComponent<TransformComponent>()) return;
    
    auto& transform = pawnEntity.GetComponent<TransformComponent>();
    
    nav::UpdatePathfindingAgent(agent_, transform, *navSystem_, pawnEntity, dt);
}

void AIController::ApplyMovementToPawn() {
    if (!pawn_ || !agent_.IsMoving()) return;
    
    Entity pawnEntity = pawn_->GetEntity();
    if (!pawnEntity.HasComponent<TransformComponent>()) return;
    
    auto& transform = pawnEntity.GetComponent<TransformComponent>();
    Vector3 currentPos = transform.Position;
    Vector3 targetWaypoint = agent_.GetCurrentWaypointPosition();
    
    Vector3 direction = targetWaypoint - currentPos;
    direction.y = 0.0f;
    
    float distance = glm::length(direction);
    if (distance > 0.1f) {
        direction = glm::normalize(direction);
        
        // Use Character::Move() directly for world-space direction
        // This bypasses the camera-relative transformation in Character::Update()
        if (auto* character = GetPawn<Character>()) {
            character->Move(direction);
            
            // Set target rotation directly on character
            float targetYaw = glm::degrees(std::atan2(direction.x, direction.z));
            character->SetTargetRotation(targetYaw);
        } else {
            // Fallback for non-Character pawns
            pawn_->AddMovementInput(direction, 1.0f);
        }
    }
}

}  // namespace se
