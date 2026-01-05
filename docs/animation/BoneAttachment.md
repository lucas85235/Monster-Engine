# Bone Attachment

The `BoneAttachment` class attaches entities (weapons, accessories) to skeleton bones.

---

## Overview

Bone attachments:
- Follow bone transforms automatically
- Support position, rotation, and scale offsets
- Can be applied to any entity

```mermaid
graph LR
    BONE[Bone Matrix] --> ATTACH[BoneAttachment]
    ATTACH --> WORLD[World Transform]
    WORLD --> ENTITY[Attached Entity]
```

---

## Configuration

```cpp
#include "engine/animation/BoneAttachment.h"

using namespace se::anim;

BoneAttachmentConfig config;
config.boneName = "hand_r";                        // Target bone
config.positionOffset = glm::vec3(0, 0.1f, 0);     // Local offset
config.rotationOffset = glm::vec3(-90, 0, 0);      // Euler degrees
config.scale = glm::vec3(1.0f);
config.inheritScale = true;
```

---

## Usage

### Initialization

```cpp
BoneAttachment rifleAttachment;
rifleAttachment.Initialize(config, skeleton);

if (!rifleAttachment.IsInitialized()) {
    SE_LOG_ERROR("Bone not found: {}", config.boneName);
}
```

### Update Loop

```cpp
void LateUpdate(float dt) {
    glm::mat4 characterWorld = GetWorldMatrix();
    
    rifleAttachment.Update(animator, characterWorld);
    rifleAttachment.ApplyToEntity(rifleEntity);
}
```

### Manual Transform Access

```cpp
glm::mat4 worldMatrix = rifleAttachment.GetWorldTransform();
glm::vec3 position = rifleAttachment.GetWorldPosition();
glm::quat rotation = rifleAttachment.GetWorldRotation();
```

---

## Adjusting Offsets

```cpp
rifleAttachment.SetPositionOffset(glm::vec3(0, 0.05f, 0));
rifleAttachment.SetRotationOffset(glm::vec3(-90, 0, 45));
rifleAttachment.SetScale(glm::vec3(0.01f));
```

---

## Multiple Attachments

```cpp
class CharacterWithEquipment : public se::Component {
    BoneAttachment helmet_;
    BoneAttachment sword_;
    BoneAttachment shield_;
    
    void Start() override {
        auto* skeleton = GetSkeleton();
        
        helmet_.Initialize({"head", {0, 0.1f, 0}}, skeleton);
        sword_.Initialize({"hand_r", {0, 0, 0}, {-90, 0, 0}}, skeleton);
        shield_.Initialize({"hand_l", {0, 0, 0}}, skeleton);
    }
    
    void LateUpdate(float dt) override {
        auto* animator = GetComponent<AnimatorComponent>().animator.get();
        glm::mat4 world = GetWorldMatrix();
        
        helmet_.Update(animator, world);
        sword_.Update(animator, world);
        shield_.Update(animator, world);
        
        helmet_.ApplyToEntity(helmetEntity_);
        sword_.ApplyToEntity(swordEntity_);
        shield_.ApplyToEntity(shieldEntity_);
    }
};
```

---

## Integration with AdvancedAnimatorComponent

```cpp
AdvancedAnimatorComponent animator;
animator.Initialize(config, skeleton);

// Add attachments
size_t rifleIndex = animator.AddAttachment({
    "hand_r",
    glm::vec3(0, 0.1f, 0),
    glm::vec3(-90, 0, 0)
});

// Access later
BoneAttachment* rifle = animator.GetAttachment(rifleIndex);
// or
BoneAttachment* rifle = animator.GetAttachment("hand_r");
```

---

## API Reference

### BoneAttachmentConfig

| Field | Type | Description |
|-------|------|-------------|
| `boneName` | `string` | Target bone name |
| `positionOffset` | `vec3` | Local position offset |
| `rotationOffset` | `vec3` | Euler rotation offset |
| `scale` | `vec3` | Scale |
| `inheritScale` | `bool` | Inherit bone scale |

### BoneAttachment

| Method | Description |
|--------|-------------|
| `Initialize(config, skeleton)` | Setup attachment |
| `IsInitialized()` | Check if valid |
| `Update(animator, worldMatrix)` | Update transform |
| `ApplyToEntity(entity)` | Apply to entity |
| `GetWorldTransform()` | Get 4x4 matrix |
| `GetWorldPosition()` | Get position |
| `GetWorldRotation()` | Get rotation |
| `GetBoneIndex()` | Get bone index |
| `GetBoneName()` | Get bone name |

---

## See Also

- [Animation Overview](Overview.md)
- [IK System](IK.md)
- [Animator](Animator.md)
