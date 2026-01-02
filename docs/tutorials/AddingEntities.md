# Adding Entities Tutorial

This tutorial covers creating and configuring entities in MonsterEngine.

---

## Creating Entities

```cpp
auto scene = std::make_unique<se::Scene>("MyScene");

// Create entity with name
se::Entity player = scene->CreateEntity("Player");

// Every entity starts with:
// - TransformComponent
// - NameComponent
```

---

## Adding Components

```cpp
// Get existing transform (auto-added)
auto& transform = player.GetComponent<se::TransformComponent>();
transform.Position = {0.0f, 5.0f, 0.0f};
transform.Rotation = {0.0f, 45.0f, 0.0f};  // Degrees
transform.Scale = {1.0f, 1.0f, 1.0f};

// Add mesh for rendering
auto& mesh = player.AddComponent<se::MeshRenderComponent>();
mesh.vertex_array = se::MeshFactory::Cube();
mesh.Color = {1.0f, 0.5f, 0.0f, 1.0f};  // Orange

// Add rigidbody for physics
auto& rb = player.AddComponent<se::RigidbodyComponent>();
// ... configure physics
```

---

## Common Entity Patterns

### Static Prop

```cpp
auto rock = scene->CreateEntity("Rock");
rock.GetComponent<se::TransformComponent>().Position = {10, 0, 5};

auto& mesh = rock.AddComponent<se::MeshRenderComponent>();
mesh.vertex_array = modelManager->Load("rock.fbx")->GetMesh(0);
mesh.CastShadows = true;
mesh.ReceiveShadows = true;
```

### Light Source

```cpp
auto sun = scene->CreateEntity("Sun");
auto& transform = sun.GetComponent<se::TransformComponent>();
transform.Rotation = {-45.0f, 30.0f, 0.0f};

auto& light = sun.AddComponent<se::DirectionalLightComponent>();
light.Color = {1.0f, 0.98f, 0.95f};
light.Intensity = 2.5f;
light.CastShadows = true;
```

### Player Character

```cpp
auto player = scene->CreateEntity("Player");

// Transform
auto& transform = player.GetComponent<se::TransformComponent>();
transform.Position = {0.0f, 1.0f, 0.0f};

// Animated mesh
auto& skinned = player.AddComponent<se::SkinnedModelComponent>();
skinned.Model = skinnedModelManager->Load("character.fbx");

// Animator
auto& anim = player.AddComponent<se::AnimatorComponent>();
anim.animator = std::make_unique<se::Animator>(skinned.Model->GetData());
anim.animator->Play(idleClip, true);

// Physics
// ... add capsule collider
```

---

## Checking & Removing

```cpp
if (entity.HasComponent<se::MeshRenderComponent>()) {
    entity.RemoveComponent<se::MeshRenderComponent>();
}
```

---

## Finding Entities

```cpp
se::Entity found = scene->FindEntityByName("Player");
if (found.IsValid()) {
    // Use entity
}
```

---

## See Also

- [Entity](../ecs/Entity.md)
- [Components](../ecs/Components.md)
