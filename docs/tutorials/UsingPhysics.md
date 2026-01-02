# Using Physics Tutorial

This tutorial demonstrates how to set up physics-enabled objects in MonsterEngine.

---

## Goal

Create a scene with:
- A static ground plane
- Falling dynamic boxes
- Raycasting to detect objects

---

## Step 1: Enable Physics for Scene

```cpp
#include <Engine.h>
#include "engine/physics/PhysicsSystem.h"
#include "engine/physics/RigidbodyComponent.h"

class PhysicsLayer : public se::Layer {
public:
    void OnAttach() override {
        // Create scene with physics enabled
        se::SceneSettings settings;
        settings.EnablePhysics = true;
        
        scene_ = std::make_unique<se::Scene>("PhysicsScene", settings);
        se::Application::Get().SetActiveScene(scene_.get());
        
        // Get physics system
        physics_ = scene_->GetPhysicsSystem();
    }
    
private:
    std::unique_ptr<se::Scene> scene_;
    se::PhysicsSystem* physics_;
};
```

---

## Step 2: Create Static Ground

Static objects have mass = 0:

```cpp
void CreateGround() {
    auto ground = scene_->CreateEntity("Ground");
    
    // Position and scale
    auto& transform = ground.GetComponent<se::TransformComponent>();
    transform.Position = {0.0f, -0.5f, 0.0f};
    transform.Scale = {50.0f, 1.0f, 50.0f};
    
    // Visual mesh
    auto& mesh = ground.AddComponent<se::MeshRenderComponent>();
    mesh.vertex_array = se::MeshFactory::Cube();
    mesh.Color = {0.3f, 0.5f, 0.3f, 1.0f};
    
    // Physics body (static)
    se::RigidbodyData data;
    data.mass = 0.0f;  // Static!
    data.shapeType = se::ColliderType::Box;
    data.boxHalfExtents = {25.0f, 0.5f, 25.0f};
    data.friction = 0.8f;
    
    auto& rb = ground.AddComponent<se::RigidbodyComponent>();
    rb.Body = physics_->AddRigidBody(ground, data);
}
```

---

## Step 3: Create Dynamic Boxes

Dynamic objects have mass > 0:

```cpp
void CreateBox(const glm::vec3& position) {
    auto box = scene_->CreateEntity("Box");
    
    // Position
    auto& transform = box.GetComponent<se::TransformComponent>();
    transform.Position = position;
    
    // Visual mesh
    auto& mesh = box.AddComponent<se::MeshRenderComponent>();
    mesh.vertex_array = se::MeshFactory::Cube();
    mesh.Color = {0.8f, 0.2f, 0.2f, 1.0f};
    
    // Physics body (dynamic)
    se::RigidbodyData data;
    data.mass = 1.0f;  // Dynamic!
    data.shapeType = se::ColliderType::Box;
    data.boxHalfExtents = {0.5f, 0.5f, 0.5f};
    data.friction = 0.5f;
    data.restitution = 0.3f;  // Bounciness
    
    auto& rb = box.AddComponent<se::RigidbodyComponent>();
    rb.Body = physics_->AddRigidBody(box, data);
}

void SpawnBoxes() {
    // Stack of boxes
    for (int y = 0; y < 5; y++) {
        CreateBox({0.0f, 2.0f + y * 1.1f, 0.0f});
    }
}
```

---

## Step 4: Raycasting

Detect objects under the mouse:

```cpp
void OnUpdate(float dt) override {
    scene_->OnUpdate(dt);
    
    // Raycast on click
    auto& input = se::InputManager::Get();
    if (input.IsMouseButtonDown(se::Mouse::ButtonLeft)) {
        DoRaycast();
    }
}

void DoRaycast() {
    // Get ray from camera through mouse position
    glm::vec3 start = camera_->GetPosition();
    glm::vec3 direction = GetMouseRayDirection();
    glm::vec3 end = start + direction * 100.0f;
    
    // Perform raycast
    glm::vec3 hitPoint, hitNormal;
    btRigidBody* hitBody = physics_->RaycastHitBody(
        start, end, hitPoint
    );
    
    if (hitBody) {
        Logger::Info("Hit at ({}, {}, {})", 
                     hitPoint.x, hitPoint.y, hitPoint.z);
        
        // Apply impulse to hit object
        hitBody->applyCentralImpulse(
            btVector3(direction.x, direction.y, direction.z) * 10.0f
        );
    }
}
```

---

## Step 5: Debug Visualization

Render physics shapes for debugging:

```cpp
void OnRender() override {
    scene_->OnRender();
    
    // Render physics debug shapes
    if (showPhysicsDebug_) {
        physics_->RenderDebug(*camera_);
    }
}

void OnImGuiRender() override {
    ImGui::Begin("Physics Debug");
    ImGui::Checkbox("Show Colliders", &showPhysicsDebug_);
    
    // Stats
    ImGui::Text("Active Bodies: %zu", physics_->GetActiveBodyCount());
    ImGui::Text("Sleeping: %zu", physics_->GetSleepingBodyCount());
    ImGui::Text("Physics Time: %.2f ms", 
                physics_->GetLastPhysicsExecutionTime());
    ImGui::End();
}
```

---

## Complete Example

```cpp
class PhysicsLayer : public se::Layer {
public:
    void OnAttach() override {
        // Scene setup
        se::SceneSettings settings;
        settings.EnablePhysics = true;
        scene_ = std::make_unique<se::Scene>("PhysicsDemo", settings);
        physics_ = scene_->GetPhysicsSystem();
        
        // Camera
        camera_ = std::make_unique<Camera>();
        camera_->SetPosition({10.0f, 10.0f, 10.0f});
        camera_->LookAt({0.0f, 0.0f, 0.0f});
        scene_->SetActiveCamera(camera_.get());
        
        // Create objects
        CreateGround();
        SpawnBoxes();
    }
    
    void OnUpdate(float dt) override {
        scene_->OnUpdate(dt);
    }
    
    void OnRender() override {
        scene_->OnRender();
        physics_->RenderDebug(*camera_);
    }
    
private:
    std::unique_ptr<se::Scene> scene_;
    std::unique_ptr<Camera> camera_;
    se::PhysicsSystem* physics_;
};
```

---

## Physics Tips

1. **Static vs Dynamic**: `mass = 0` means static
2. **Sleeping**: Bodies automatically sleep when at rest
3. **Shape Matching**: Collider should match visual mesh
4. **Performance**: Use simple shapes (Box, Sphere, Capsule)
5. **Debug Draw**: Essential for debugging physics issues

---

## See Also

- [Physics Overview](../physics/Overview.md)
- [PhysicsSystem](../physics/PhysicsSystem.md)
- [Rigidbodies](../physics/Rigidbodies.md)
