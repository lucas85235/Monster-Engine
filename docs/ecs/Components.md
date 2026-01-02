# Components Reference

This document provides a complete reference for all built-in components in MonsterEngine.

---

## Table of Contents

1. [TransformComponent](#transformcomponent)
2. [NameComponent](#namecomponent)
3. [MeshRenderComponent](#meshrendercomponent)
4. [DirectionalLightComponent](#directionallightcomponent)
5. [SpringArmComponent](#springarmcomponent)
6. [RelationshipComponent](#relationshipcomponent)

---

## TransformComponent

The core transform component providing position, rotation, and scale with cached matrix optimization.

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `Position` | `Vector3` | `{0, 0, 0}` | World position |
| `Rotation` | `Vector3` | `{0, 0, 0}` | Euler angles in degrees |
| `Scale` | `Vector3` | `{1, 1, 1}` | Scale factors |
| `WorldMatrix` | `Matrix4` | `identity` | Calculated world transform (for hierarchies) |

### Methods

```cpp
// Get the transformation matrix (cached)
Matrix4 GetTransform() const;

// Setters (automatically mark cache dirty)
void SetPosition(const Vector3& position);
void SetRotation(const Vector3& rotation);  // Degrees
void SetScale(const Vector3& scale);

// Movement helpers
void Translate(const Vector3& offset);
void Rotate(const Vector3& offset);  // Degrees

// Direction vectors
Vector3 GetForward() const;  // -Z axis
Vector3 GetRight() const;    // +X axis
Vector3 GetUp() const;       // +Y axis

// Quaternion access
Quaternion GetQuaternion() const;

// Force cache invalidation
void MarkDirty();
```

### Example

```cpp
auto& transform = entity.GetComponent<se::TransformComponent>();

// Set position
transform.SetPosition({10.0f, 0.0f, 5.0f});

// Rotate 45 degrees around Y
transform.SetRotation({0.0f, 45.0f, 0.0f});

// Move forward
Vector3 forward = transform.GetForward();
transform.Translate(forward * speed * deltaTime);

// Get final matrix for rendering
Matrix4 modelMatrix = transform.GetTransform();
```

### Cache Optimization

The component caches the transformation matrix. Setting properties through setters automatically invalidates the cache. If you modify `Position`, `Rotation`, or `Scale` directly, call `MarkDirty()`:

```cpp
transform.Position.x += 1.0f;  // Direct access
transform.MarkDirty();         // Invalidate cache
```

---

## NameComponent

A simple string tag for identifying entities.

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `Name` | `std::string` | `""` | Entity name |

### Example

```cpp
auto& name = entity.GetComponent<se::NameComponent>();
name.Name = "Player";

// Implicit conversion to string
std::string entityName = entity.GetComponent<se::NameComponent>();
```

---

## MeshRenderComponent

Defines a renderable 3D mesh with material properties.

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `vertex_array` | `shared_ptr<VertexArray>` | `nullptr` | Mesh geometry |
| `material` | `shared_ptr<Material>` | `nullptr` | Base material |
| `materialInstance` | `MaterialInstance*` | `nullptr` | Instance from MaterialLibrary |
| `Color` | `Vector4` | `{1,1,1,1}` | Per-instance color tint |
| `IsVisible` | `bool` | `true` | Render this mesh |
| `CastShadows` | `bool` | `true` | Include in shadow pass |
| `ReceiveShadows` | `bool` | `true` | Sample shadow maps |

#### PBR Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `Metallic` | `float` | `0.0` | Metalness (0=dielectric, 1=metal) |
| `Roughness` | `float` | `0.5` | Surface roughness [0-1] |
| `Reflectance` | `float` | `0.5` | Dielectric reflectance |
| `AO` | `float` | `1.0` | Ambient occlusion multiplier |
| `UseCustomPBR` | `bool` | `false` | Use above PBR values |

#### Emissive Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `EmissiveColor` | `Vector3` | `{0,0,0}` | Emissive RGB |
| `EmissiveFactor` | `float` | `0.0` | Emission intensity |

### Example

```cpp
auto& mesh = entity.AddComponent<se::MeshRenderComponent>();

// Set geometry
mesh.vertex_array = se::MeshFactory::Sphere(32, 32);

// Set material
mesh.material = myPBRMaterial;

// Configure PBR
mesh.UseCustomPBR = true;
mesh.Metallic = 1.0f;
mesh.Roughness = 0.2f;

// Make it glow
mesh.EmissiveColor = {1.0f, 0.5f, 0.0f};
mesh.EmissiveFactor = 5.0f;

// Shadow settings
mesh.CastShadows = true;
mesh.ReceiveShadows = true;
```

---

## DirectionalLightComponent

Defines a directional light (sun-like) for scene lighting.

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `Color` | `Vector3` | `{1,1,1}` | Light color |
| `Intensity` | `float` | `1.0` | Light intensity |
| `Enabled` | `bool` | `true` | Is light active |
| `CastShadows` | `bool` | `true` | Generate shadow maps |

### Example

```cpp
auto sun = scene->CreateEntity("Sun");

auto& transform = sun.GetComponent<se::TransformComponent>();
transform.SetRotation({-45.0f, 30.0f, 0.0f});  // Light direction from rotation

auto& light = sun.AddComponent<se::DirectionalLightComponent>();
light.Color = {1.0f, 0.95f, 0.9f};
light.Intensity = 2.0f;
light.CastShadows = true;
```

The light direction is derived from the entity's `TransformComponent` rotation.

---

## SpringArmComponent

Third-person camera boom with collision detection, similar to Unreal Engine's spring arm.

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `TargetArmLength` | `float` | `5.0` | Desired arm length |
| `SocketOffset` | `Vector3` | `{0,0,0}` | Offset from target |
| `Pitch` | `float` | `-20.0` | Vertical rotation (degrees) |
| `Yaw` | `float` | `0.0` | Horizontal rotation (degrees) |
| `MinPitch` | `float` | `-80.0` | Minimum pitch limit |
| `MaxPitch` | `float` | `80.0` | Maximum pitch limit |
| `DoCollisionTest` | `bool` | `true` | Prevent camera clipping |
| `ProbeSize` | `float` | `0.12` | Collision probe radius |
| `CollisionLag` | `float` | `0.0` | Smooth collision response |
| `CurrentArmLength` | `float` | `5.0` | Actual length (after collision) |

### Example

```cpp
auto cameraEntity = scene->CreateEntity("CameraBoom");

auto& springArm = cameraEntity.AddComponent<se::SpringArmComponent>();
springArm.TargetArmLength = 8.0f;
springArm.SocketOffset = {0.0f, 1.5f, 0.0f};  // Above character
springArm.DoCollisionTest = true;

// Control with mouse input
void OnUpdate(float dt) {
    springArm.Yaw += mouseDeltaX * sensitivity;
    springArm.Pitch = std::clamp(
        springArm.Pitch + mouseDeltaY * sensitivity,
        springArm.MinPitch,
        springArm.MaxPitch
    );
}
```

---

## RelationshipComponent

Parent-child hierarchy for entities.

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `Parent` | `entt::entity` | `entt::null` | Parent entity handle |
| `Children` | `vector<entt::entity>` | `{}` | Child entity handles |

### Methods

```cpp
size_t ChildrenCount() const;
```

### Example

```cpp
auto parent = scene->CreateEntity("Parent");
auto child = scene->CreateEntity("Child");

// Set up relationship
auto& childRel = child.AddComponent<se::RelationshipComponent>();
childRel.Parent = parent.GetHandle();

auto& parentRel = parent.AddComponent<se::RelationshipComponent>();
parentRel.Children.push_back(child.GetHandle());
```

When the scene updates transforms, child world matrices are computed relative to their parent:

```cpp
worldMatrix = parent.WorldMatrix * child.GetTransform();
```

---

## Creating Custom Components

Components are just plain structs:

```cpp
// In your header
struct VelocityComponent {
    glm::vec3 Linear{0.0f};
    glm::vec3 Angular{0.0f};
    float Damping = 0.98f;
};

struct HealthComponent {
    float Current = 100.0f;
    float Maximum = 100.0f;
    
    bool IsAlive() const { return Current > 0.0f; }
    float Percentage() const { return Current / Maximum; }
    
    void TakeDamage(float amount) {
        Current = std::max(0.0f, Current - amount);
    }
};

// Usage
entity.AddComponent<VelocityComponent>();
entity.AddComponent<HealthComponent>();
```

Guidelines for custom components:
- Keep them as plain data structs
- Small helper methods are fine
- Avoid complex logic—put that in systems
- Use default member initializers

---

## See Also

- [Entity](Entity.md)
- [Scene](Scene.md)
- [Tutorial: Custom Components](../tutorials/CustomComponents.md)
