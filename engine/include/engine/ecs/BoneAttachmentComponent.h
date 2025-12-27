#pragma once

#include <glm.hpp>
#include <string>

#include "engine/ecs/Entity.h"

namespace se {

/**
 * BoneAttachmentComponent - Attaches an entity to a bone of an animated character.
 * 
 * Usage:
 *   Entity weapon = scene->CreateEntity("Sword");
 *   weapon.AddComponent<BoneAttachmentComponent>(characterEntity, "RightHand");
 */
struct BoneAttachmentComponent {
    Entity TargetEntity;
    std::string BoneName;
    
    glm::vec3 PositionOffset{0.0f};
    glm::vec3 RotationOffset{0.0f};  // Euler angles in degrees
    glm::vec3 ScaleMultiplier{1.0f};
    
    bool Active = true;
    
    BoneAttachmentComponent() = default;
    BoneAttachmentComponent(Entity target, const std::string& boneName)
        : TargetEntity(target), BoneName(boneName) {}
    BoneAttachmentComponent(Entity target, const std::string& boneName, 
                            const glm::vec3& posOffset, const glm::vec3& rotOffset = glm::vec3(0.0f))
        : TargetEntity(target), BoneName(boneName), 
          PositionOffset(posOffset), RotationOffset(rotOffset) {}
};

}  // namespace se
