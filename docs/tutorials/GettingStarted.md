# Getting Started

This tutorial walks you through creating your first MonsterEngine application.

---

## Prerequisites

- CMake 3.16+
- C++17 compiler
- OpenGL 4.5 capable GPU

---

## Project Setup

### 1. Create Project Structure

```
MyGame/
├── CMakeLists.txt
├── src/
│   └── main.cpp
└── assets/
    └── shaders/
```

### 2. CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyGame)

set(CMAKE_CXX_STANDARD 17)

# Add the engine
add_subdirectory(path/to/SimpleEngine)

# Create executable
add_executable(MyGame src/main.cpp)

# Link engine
target_link_libraries(MyGame PRIVATE SimpleEngine)
```

---

## Your First Application

### main.cpp

```cpp
#include <Engine.h>

class GameLayer : public se::Layer {
public:
    GameLayer() : Layer("GameLayer") {}

    void OnAttach() override {
        // Initialize your game here
        Logger::Info("Game started!");
        
        // Create a scene
        scene_ = std::make_unique<se::Scene>("MainScene");
        se::Application::Get().SetActiveScene(scene_.get());
        
        // Setup camera
        camera_ = std::make_unique<Camera>();
        camera_->SetPerspective(45.0f, 0.1f, 1000.0f);
        camera_->SetPosition({0, 5, 10});
        camera_->LookAt({0, 0, 0});
        scene_->SetActiveCamera(camera_.get());
        
        // Create a cube
        auto cube = scene_->CreateEntity("Cube");
        auto& mesh = cube.AddComponent<se::MeshRenderComponent>();
        mesh.vertex_array = se::MeshFactory::Cube();
    }

    void OnDetach() override {
        Logger::Info("Game ended!");
    }

    void OnUpdate(float deltaTime) override {
        // Update game logic
        scene_->OnUpdate(deltaTime);
        
        // Simple camera controls
        auto& input = se::InputManager::Get();
        float speed = 5.0f * deltaTime;
        
        if (input.IsKeyDown(se::Key::W)) camera_->Move({0, 0, -speed});
        if (input.IsKeyDown(se::Key::S)) camera_->Move({0, 0, speed});
        if (input.IsKeyDown(se::Key::A)) camera_->Move({-speed, 0, 0});
        if (input.IsKeyDown(se::Key::D)) camera_->Move({speed, 0, 0});
    }

    void OnRender() override {
        scene_->OnRender();
    }

    void OnImGuiRender() override {
        ImGui::Begin("Debug");
        ImGui::Text("FPS: %.1f", 1.0f / ImGui::GetIO().DeltaTime);
        ImGui::End();
    }

private:
    std::unique_ptr<se::Scene> scene_;
    std::unique_ptr<Camera> camera_;
};

int main() {
    se::ApplicationSpecification spec;
    spec.Name = "My First Game";
    spec.WindowWidth = 1280;
    spec.WindowHeight = 720;
    spec.VSync = true;
    
    se::Application app(spec);
    app.PushLayer<GameLayer>();
    return app.Run();
}
```

---

## Build and Run

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
./MyGame
```

---

## What You'll See

- A window with a 3D cube
- WASD camera movement
- ImGui debug panel showing FPS

---

## Next Steps

1. **Add More Objects**: [Adding Entities](AddingEntities.md)
2. **Custom Logic**: [Custom Components](CustomComponents.md)
3. **Post-Processing**: [Renderer Overview](../renderer/Overview.md)
4. **Physics**: [Using Physics](UsingPhysics.md)

---

## Understanding the Code

### ApplicationSpecification
Configures window settings before launch.

### Layer
Your game logic lives in Layer subclasses. Each layer has lifecycle methods:
- `OnAttach()`: Initialize resources
- `OnDetach()`: Cleanup
- `OnUpdate()`: Game logic (called every frame)
- `OnRender()`: Rendering (after update)
- `OnImGuiRender()`: Debug UI

### Scene
Container for entities and systems.

### Entity
Game objects with components.

### Camera
View into the 3D world.

---

## See Also

- [Application](../core/Application.md)
- [Scene](../ecs/Scene.md)
- [Entity](../ecs/Entity.md)
