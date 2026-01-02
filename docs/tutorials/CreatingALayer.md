# Creating a Layer Tutorial

Layers are the primary way to organize game logic in MonsterEngine.

---

## Basic Layer

```cpp
#include <Engine.h>

class GameLayer : public se::Layer {
public:
    GameLayer() : Layer("GameLayer") {}

    void OnAttach() override {
        // Called when layer is added
        Logger::Info("GameLayer attached");
    }

    void OnDetach() override {
        // Called when layer is removed
    }

    void OnUpdate(float dt) override {
        // Called every frame - game logic here
    }

    void OnRender() override {
        // Called after all OnUpdate - rendering here
    }

    void OnImGuiRender() override {
        // Called during ImGui frame - debug UI here
        ImGui::Begin("Debug");
        ImGui::End();
    }

    void OnEvent(se::Event& event) override {
        // Handle window/input events
    }
};

// In main.cpp
app.PushLayer<GameLayer>();
```

---

## Layer with Scene

```cpp
class GameLayer : public se::Layer {
public:
    void OnAttach() override {
        scene_ = std::make_unique<se::Scene>("Game");
        se::Application::Get().SetActiveScene(scene_.get());
        
        // Setup camera, entities, etc.
    }

    void OnUpdate(float dt) override {
        scene_->OnUpdate(dt);
    }

    void OnRender() override {
        scene_->OnRender();
    }

private:
    std::unique_ptr<se::Scene> scene_;
};
```

---

## Layer vs Overlay

```cpp
app.PushLayer<GameLayer>();    // Processed first
app.PushOverlay<DebugLayer>(); // Processed last (renders on top)
```

---

## See Also

- [Application](../core/Application.md)
