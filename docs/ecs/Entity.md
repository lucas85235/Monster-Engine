# Entity

The `Entity` class is a lightweight wrapper around an entt entity handle. It provides a convenient API for component management.

---

## Overview

An entity is just an ID—a handle into the ECS registry. The `Entity` class wraps this handle along with a pointer to its owning `Scene`, providing a clean API:

```cpp
se::Entity player = scene->CreateEntity("Player");

// Add components
player.AddComponent<se::MeshRenderComponent>();

// Get components  
auto& transform = player.GetComponent<se::TransformComponent>();

// Check components
if (player.HasComponent<se::RigidbodyComponent>()) { }

// Remove components
player.RemoveComponent<se::MeshRenderComponent>();
```

---

## Creating Entities

Entities are created through the `Scene`:

```cpp
// Create with auto-generated name
se::Entity entity = scene->CreateEntity();

// Create with specific name
se::Entity player = scene->CreateEntity("Player");
```

Every entity is automatically created with:
- `TransformComponent` - Position, rotation, scale
- `NameComponent` - Entity name

---

## Component Operations

### Adding Components

```cpp
// Add with default values
auto& mesh = entity.AddComponent<se::MeshRenderComponent>();
mesh.vertex_array = myMesh;
mesh.material = myMaterial;

// Add with constructor arguments (if component supports it)
auto& name = entity.AddComponent<se::NameComponent>("NewName");
```

### Getting Components

```cpp
// Get reference (throws if component doesn't exist)
auto& transform = entity.GetComponent<se::TransformComponent>();
transform.Position = {10.0f, 0.0f, 0.0f};

// Multiple components at once
auto [transform, mesh] = entity.GetComponents<
    se::TransformComponent,
    se::MeshRenderComponent
>();
```

### Checking Components

```cpp
if (entity.HasComponent<se::RigidbodyComponent>()) {
    auto& rb = entity.GetComponent<se::RigidbodyComponent>();
    // Safe to access
}
```

### Removing Components

```cpp
entity.RemoveComponent<se::MeshRenderComponent>();
// Component is immediately destroyed
```

> **Note**: You cannot remove `TransformComponent` or `NameComponent`.

---

## Validity Checks

Entities can become invalid if destroyed or if created with a null scene:

```cpp
se::Entity entity = scene->CreateEntity("Test");

if (entity.IsValid()) {
    // Safe to use
}

// Also works with bool conversion
if (entity) {
    // Valid
}

// After destruction
scene->DestroyEntity(entity);
if (!entity.IsValid()) {
    // Now invalid
}
```

---

## Comparing Entities

```cpp
se::Entity a = scene->CreateEntity("A");
se::Entity b = scene->CreateEntity("B");
se::Entity c = a; // Copy

if (a == c) {
    // True - same entity
}

if (a != b) {
    // True - different entities
}
```

---

## Getting Entity Handle

For advanced entt operations, you can get the raw handle:

```cpp
entt::entity handle = entity.GetHandle();

// Use with raw registry operations
entt::registry& registry = scene->GetRegistry();
registry.emplace<CustomComponent>(handle);
```

---

## Entity Relationships

Using `RelationshipComponent`, entities can form hierarchies:

```cpp
// Create parent-child relationship
auto parent = scene->CreateEntity("Parent");
auto child = scene->CreateEntity("Child");

// Add relationship component
auto& rel = child.AddComponent<se::RelationshipComponent>();
rel.Parent = parent.GetHandle();

// Parent tracks children
auto& parentRel = parent.AddComponent<se::RelationshipComponent>();
parentRel.Children.push_back(child.GetHandle());
```

World transforms are calculated considering parent transforms. See [TransformComponent](Components.md#transformcomponent) for details.

---

## Example: Complete Entity Setup

```cpp
// Create a complete game entity
se::Entity CreateCharacter(se::Scene* scene, const std::string& name) {
    auto entity = scene->CreateEntity(name);
    
    // Configure transform
    auto& transform = entity.GetComponent<se::TransformComponent>();
    transform.Position = {0.0f, 0.0f, 0.0f};
    transform.Scale = {1.0f, 1.0f, 1.0f};
    
    // Add mesh
    auto& mesh = entity.AddComponent<se::MeshRenderComponent>();
    mesh.vertex_array = LoadMesh("character.fbx");
    mesh.material = LoadMaterial("character.mat");
    mesh.CastShadows = true;
    mesh.ReceiveShadows = true;
    
    // Add physics
    se::RigidbodyData rbData;
    rbData.mass = 80.0f;
    rbData.shapeType = se::ColliderType::Capsule;
    rbData.capsuleRadius = 0.5f;
    rbData.capsuleHeight = 1.8f;
    
    auto& rb = entity.AddComponent<se::RigidbodyComponent>();
    scene->GetPhysicsSystem()->AddRigidBody(entity, rbData);
    
    // Add custom components
    entity.AddComponent<HealthComponent>();
    entity.AddComponent<CharacterController>();
    
    return entity;
}
```

---

## API Reference

### Constructors

```cpp
Entity();                              // Invalid entity
Entity(entt::entity handle, Scene* scene);  // From handle
```

### Methods

| Method | Description |
|--------|-------------|
| `AddComponent<T>(args...)` | Add component, returns reference |
| `GetComponent<T>()` | Get component reference (throws if missing) |
| `GetComponents<T1, T2, ...>()` | Get multiple components as tuple |
| `HasComponent<T>()` | Check if component exists |
| `RemoveComponent<T>()` | Remove component |
| `IsValid()` | Check if entity is valid |
| `GetHandle()` | Get raw entt::entity handle |
| `operator bool()` | Same as IsValid() |
| `operator==` / `operator!=` | Compare entities |

---

## See Also

- [Scene](Scene.md)
- [Components](Components.md)
- [Tutorial: Adding Entities](../tutorials/AddingEntities.md)
