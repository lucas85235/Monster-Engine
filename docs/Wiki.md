# MonsterEngine Wiki

Welcome to the MonsterEngine documentation. This wiki covers the internal architecture, how to create new content, and how to extend the engine.

## Table of Contents
1. [Architecture Overview](#architecture-overview)
2. [Core Modules](#core-modules)
3. [Developer Guides](#developer-guides)

---

## Architecture Overview

MonsterEngine follows a **Layered Architecture**. The `Application` class maintains a `LayerStack`. Every frame, the application iterates through this stack to update and render each layer.

### The Application Loop
1. **Poll Events**: Input and window events are gathered.
2. **Update Layers**: `OnUpdate(timestep)` is called for each layer.
3. **Render Layers**: `OnRender()` is called for each layer.
4. **ImGui Render**: `OnImGuiRender()` is called for debug UI.
5. **Swap Buffers**: The frame is presented to the screen.

For visual diagrams of this flow, see [Architecture Diagrams](Architecture.md).

---

## Core Modules

### 1. Renderer
The renderer is designed to be stateless from the user's perspective. You `Submit` geometry and materials, and the `SceneRenderer` handles the sorting, batching (future), and drawing.

- **SceneRenderer**: High-level renderer that handles lights, shadows, and scene submission.
- **Renderer**: Low-level wrapper around OpenGL commands.
- **Material**: Defines the shader and uniforms for an object.

### 2. Entity Component System (ECS)
We use `entt` for our ECS.
- **Scene**: Contains the `entt::registry`.
- **Entity**: A lightweight wrapper around an `entt::entity` handle and the `Scene`.
- **Components**: Pure data structs (e.g., `TransformComponent`, `MeshRenderComponent`).
- **Systems**: Logic is typically implemented in `OnUpdate` functions within layers or dedicated system classes.

### 3. Input System
The `InputManager` provides a polled input interface.
- `InputManager::Get().IsKeyDown(Key::A)`
- `InputManager::Get().GetMousePosition()`

---

## Developer Guides

### Creating a New Layer
Layers are the primary way to add game logic or tools.

```cpp
#include "engine/Layer.h"

class MyGameLayer : public se::Layer {
public:
    MyGameLayer() : Layer("MyGameLayer") {}

    void OnAttach() override {
        // Initialize resources
    }

    void OnUpdate(float ts) override {
        // Update logic
    }

    void OnEvent(se::Event& event) override {
        // Handle events
    }
};

// In your Application setup:
PushLayer<MyGameLayer>();
```

### Creating Components and Entities

To create a new entity and add components:

```cpp
// 1. Create Entity
se::Entity myEntity = activeScene->CreateEntity("Player");

// 2. Add Transform Component
auto& transform = myEntity.AddComponent<se::TransformComponent>();
transform.Translation = {0.0f, 5.0f, 0.0f};

// 3. Add Mesh Component
auto& mesh = myEntity.AddComponent<se::MeshRenderComponent>();
mesh.Mesh = se::MeshFactory::Cube();
mesh.Material = std::make_shared<se::Material>(shader);
```

### Creating a New Component
Define a struct in a header file (e.g., `Components.h` or a new file).

```cpp
struct HealthComponent {
    float Health = 100.0f;
    float MaxHealth = 100.0f;
};
```

Then you can immediately use it:
```cpp
myEntity.AddComponent<HealthComponent>();
```

### Creating a New System
Systems can be simple functions or classes that iterate over views.

```cpp
void HealthSystem::OnUpdate(se::Scene* scene, float dt) {
    auto view = scene->GetAllEntitiesWith<TransformComponent, HealthComponent>();
    
    for (auto entity : view) {
        auto [transform, health] = view.get<TransformComponent, HealthComponent>(entity);
        
        if (health.Health <= 0) {
            // Handle death
        }
    }
}
```
