# Pose

The `Pose` class represents a complete skeleton pose with all bone transforms.

---

## Overview

A Pose contains:
- Local `BoneTransform` for each bone (position, rotation, scale)
- Methods for blending between poses
- Sampling from animation clips

---

## BoneTransform

```cpp
struct BoneTransform {
    glm::vec3 position{0.0f};
    glm::quat rotation{1, 0, 0, 0};
    glm::vec3 scale{1.0f};
    
    static BoneTransform Identity();
    static BoneTransform Interpolate(const BoneTransform& a, 
                                     const BoneTransform& b, 
                                     float t);
};
```

---

## Creating Poses

```cpp
#include "engine/animation/advanced/Pose.h"

using namespace se::anim;

// Create pose with bone count
Pose pose;
pose.Resize(skeleton->Bones.size());

// Set to identity
pose.SetIdentity();
```

---

## Sampling from Clips

```cpp
Pose pose;
pose.Resize(skeleton->Bones.size());

// Sample animation at specific time
pose.SampleFromClip(animationClip.get(), timeInSeconds, skeleton);
```

---

## Blending

```cpp
Pose poseA, poseB;
// ... fill poses ...

// Blend B into A
poseA.BlendWith(poseB, 0.5f);  // 50% blend

// Or get blend result
Pose result = Pose::Blend(poseA, poseB, weight);
```

---

## Accessing Transforms

```cpp
// By index
BoneTransform& bone = pose.GetBoneTransform(boneIndex);
bone.rotation = glm::quat(glm::vec3(0, angle, 0));

// Const access
const BoneTransform& bone = pose.GetBoneTransform(boneIndex);
```

---

## API Reference

### Pose

| Method | Description |
|--------|-------------|
| `Resize(count)` | Set bone count |
| `GetBoneCount()` | Get bone count |
| `IsEmpty()` | Check if empty |
| `SetIdentity()` | Reset all to identity |
| `GetBoneTransform(i)` | Get bone transform |
| `SampleFromClip(clip, time, skeleton)` | Sample animation |
| `BlendWith(other, weight)` | Blend in place |
| `Blend(a, b, weight)` | Static blend |

### BoneTransform

| Method | Description |
|--------|-------------|
| `Identity()` | Get identity transform |
| `Interpolate(a, b, t)` | Interpolate transforms |

---

## See Also

- [Animation Overview](Overview.md)
- [Animator](Animator.md)
- [Animation Graph](AnimationGraph.md)
