# Creating Custom Components Tutorial

This tutorial shows how to create custom components and systems in MonsterEngine.

---

## Goal

Create a custom `HealthComponent` and update system.

---

## Step 1: Define the Component

Components are plain structs:

```cpp
// HealthComponent.h
#pragma once

namespace game {

struct HealthComponent {
    float Current = 100.0f;
    float Maximum = 100.0f;
    float RegenRate = 0.0f;  // Health per second
    
    bool IsAlive() const { 
        return Current > 0.0f; 
    }
    
    float GetPercent() const { 
        return Maximum > 0 ? Current / Maximum : 0.0f; 
    }
    
    void TakeDamage(float amount) {
        Current = std::max(0.0f, Current - amount);
    }
    
    void Heal(float amount) {
        Current = std::min(Maximum, Current + amount);
    }
};

}  // namespace game
```

---

## Step 2: Use the Component

```cpp
#include "HealthComponent.h"

// Add to entity
auto player = scene->CreateEntity("Player");
auto& health = player.AddComponent<game::HealthComponent>();
health.Maximum = 200.0f;
health.Current = 200.0f;
health.RegenRate = 5.0f;  // 5 HP/sec

// Use in logic
void OnPlayerHit(se::Entity player, float damage) {
    if (!player.HasComponent<game::HealthComponent>()) return;
    
    auto& health = player.GetComponent<game::HealthComponent>();
    health.TakeDamage(damage);
    
    if (!health.IsAlive()) {
        HandlePlayerDeath(player);
    }
}
```

---

## Step 3: Create an Update System

Process all entities with the component:

```cpp
// HealthSystem.h
#pragma once

#include "HealthComponent.h"
#include "engine/ecs/Scene.h"

namespace game {

class HealthSystem {
public:
    static void Update(se::Scene* scene, float dt) {
        auto view = scene->GetAllEntitiesWith<HealthComponent>();
        
        for (auto entityHandle : view) {
            auto& health = view.get<HealthComponent>(entityHandle);
            
            // Regenerate health
            if (health.RegenRate > 0.0f && health.IsAlive()) {
                health.Heal(health.RegenRate * dt);
            }
            
            // Check for death
            if (!health.IsAlive()) {
                // Handle death...
            }
        }
    }
};

}  // namespace game
```

---

## Step 4: Call from Layer

```cpp
void OnUpdate(float dt) override {
    // Update engine systems
    scene_->OnUpdate(dt);
    
    // Update game systems
    game::HealthSystem::Update(scene_.get(), dt);
}
```

---

## Example: Complete Health System

```cpp
// HealthComponent.h
#pragma once

#include <functional>

namespace game {

struct HealthComponent {
    float Current = 100.0f;
    float Maximum = 100.0f;
    float RegenRate = 0.0f;
    float RegenDelay = 3.0f;      // Seconds after damage
    float TimeSinceDamage = 0.0f;
    
    std::function<void()> OnDeath;
    std::function<void(float)> OnDamage;
    
    bool IsAlive() const { return Current > 0.0f; }
    float GetPercent() const { return Current / Maximum; }
    
    void TakeDamage(float amount) {
        if (!IsAlive()) return;
        
        Current = std::max(0.0f, Current - amount);
        TimeSinceDamage = 0.0f;
        
        if (OnDamage) OnDamage(amount);
        if (!IsAlive() && OnDeath) OnDeath();
    }
    
    void Heal(float amount) {
        Current = std::min(Maximum, Current + amount);
    }
};

}  // namespace game
```

```cpp
// HealthSystem.h
#pragma once

#include "HealthComponent.h"
#include "engine/ecs/Scene.h"

namespace game {

class HealthSystem {
public:
    static void Update(se::Scene* scene, float dt) {
        auto view = scene->GetAllEntitiesWith<HealthComponent>();
        
        for (auto entityHandle : view) {
            auto& health = view.get<HealthComponent>(entityHandle);
            
            if (!health.IsAlive()) continue;
            
            // Track time since damage
            health.TimeSinceDamage += dt;
            
            // Regen after delay
            if (health.RegenRate > 0.0f && 
                health.TimeSinceDamage >= health.RegenDelay) {
                health.Heal(health.RegenRate * dt);
            }
        }
    }
};

}  // namespace game
```

```cpp
// Usage
auto player = scene->CreateEntity("Player");
auto& health = player.AddComponent<game::HealthComponent>();
health.Maximum = 100.0f;
health.Current = 100.0f;
health.RegenRate = 10.0f;
health.RegenDelay = 3.0f;

health.OnDeath = [&]() {
    Logger::Info("Player died!");
    ShowGameOver();
};

health.OnDamage = [&](float damage) {
    Logger::Info("Player took {} damage!", damage);
    PlayHitSound();
    ShakeCamera();
};
```

---

## Tips

1. **Keep Data Simple**: Components are data, systems are logic
2. **No Heavy Logic**: Use simple getter methods only
3. **Callbacks Optional**: Use std::function for events
4. **View Iteration**: Use `GetAllEntitiesWith<>()` for efficient queries

---

## See Also

- [Components Reference](../ecs/Components.md)
- [ECS Overview](../ecs/Overview.md)
- [Entity](../ecs/Entity.md)
