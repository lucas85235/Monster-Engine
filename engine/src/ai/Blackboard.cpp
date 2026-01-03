#include "engine/ai/Blackboard.h"
#include "engine/ecs/Entity.h"

namespace se {

void Blackboard::SetTarget(Entity target) {
    Set("Target", target);
}

Entity Blackboard::GetTarget() const {
    return Get<Entity>("Target", Entity{});
}

}  // namespace se
