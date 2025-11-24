#include "engine/ecs/Entity.h"

#include "engine/ecs/Scene.h"

namespace se {

Entity::Entity(entt::entity handle, Scene* scene) : entityHandle_(handle), scene_(scene) {}

}  // namespace se