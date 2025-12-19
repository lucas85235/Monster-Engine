# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Project Overview

MonsterEngine is a lightweight, modular 3D game engine written in C++ with a focus on simplicity and learning game engine architecture. It features:
- Layered architecture for organizing game logic and tools
- Entity Component System (ECS) using `entt` library
- Modern OpenGL renderer with shader support, materials, and shadows
- Event system (both legacy and new EventBus)
- Bullet physics integration
- ImGui for debug UI

## Build Commands

### MacOS/Linux

**Full build and run:**
```bash
./scripts/run.sh
```
This script:
1. Creates the `build/` directory if needed
2. Runs CMake with Ninja generator and exports compile commands
3. Compiles the project
4. Automatically runs the sandbox application

**First-time setup (Linux only):**
```bash
chmod +x scripts/setup.sh
./scripts/setup.sh
```
Installs system dependencies: OpenGL, Wayland, XKB, Xrandr, Xinerama, Xcursor, XInput, zlib.

**Build manually:**
```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja
cmake --build build
```

**Run sandbox manually:**
```bash
./build/apps/sandbox/sandbox
```

### Windows

**Full build and run:**
```cmd
scripts\generate_visual_studio_files_and_build.bat
```

**Run sandbox manually:**
```cmd
.\build\apps\sandbox\Debug\Sandbox.exe
```

### Build System Notes

- CMake 3.16+ required
- C++23 standard
- Build output in `build/` directory
- Assets automatically copied to binary directory during build
- The engine is built as a static library (`simple_engine`)

## Code Formatting and Style

### Code Formatting

**Format all files (Windows only):**
```cmd
scripts\format-all-project.bat
```

**Manual formatting:**
```bash
clang-format -i path/to/file.cpp
```

The project uses a Google-based style with customizations defined in `.clang-format`:
- 4-space indentation
- 150 character line limit
- Attached braces (`{` on same line)
- Align consecutive assignments and declarations
- Pointer alignment to the left (`Type* ptr`)

### Naming Conventions (from .clang-tidy)

- **Classes/Structs/Enums**: `CamelCase` (e.g., `TransformComponent`, `PhysicsSystem`)
- **Functions/Methods**: `CamelCase` (e.g., `OnUpdate`, `CreateEntity`)
- **Variables**: `lower_case` (e.g., `delta_time`, `entity_handle`)
- **Member variables**: `lower_case` with trailing underscore (e.g., `registry_`, `layer_stack_`)
- **Parameters**: `lower_case` (e.g., `delta_time`, `event`)

### Namespace Convention

All engine code lives in the `se::` namespace (short for "SimpleEngine").

## Architecture

### Layered Architecture

The engine uses a **Layer Stack** pattern. The `Application` class maintains layers that are updated and rendered each frame.

**Application Loop (each frame):**
1. Poll input and window events
2. Call `OnUpdate(timestep)` for each layer
3. Begin renderer frame
4. Call `OnRender()` for each layer (if implemented)
5. End renderer frame
6. Render ImGui (`OnImGuiRender()` for each layer)
7. Swap buffers

**Creating a new layer:**
```cpp
#include "engine/core/Layer.h"

class MyGameLayer : public se::Layer {
public:
    MyGameLayer() : Layer("MyGameLayer") {}
    
    void OnAttach() override {
        // Initialize resources
    }
    
    void OnUpdate(float ts) override {
        // Update logic each frame
    }
    
    void OnEvent(se::Event& event) override {
        // Handle events (optional)
    }
    
    void OnImGuiRender() override {
        // Debug UI (optional)
    }
};

// In Application setup (main.cpp):
application.PushLayer<MyGameLayer>();
```

**Layer vs Overlay:**
- `PushLayer()`: Regular layers, inserted at layer_insert_index
- `PushOverlay()`: Always rendered on top (e.g., ImGuiLayer)

### Entity Component System (ECS)

The engine uses `entt` for ECS. Key classes:
- `Scene`: Contains the `entt::registry` and manages entity lifecycle
- `Entity`: Lightweight wrapper around `entt::entity` handle
- Components: Pure data structs (POD)
- Systems: Logic that operates on entities with specific components

**Core Components** (in `engine/include/engine/ecs/Components.h`):
- `TransformComponent`: Position, Rotation (Euler angles in degrees), Scale
- `NameComponent`: Human-readable entity name
- `MeshRenderComponent`: VertexArray, Material, visibility flags
- `DirectionalLightComponent`: Color, intensity, shadow casting
- `SpringArmComponent`: Third-person camera control

**Physics Components** (in `engine/include/engine/physics/`):
- `RigidbodyComponent`: Mass, velocity, physics body reference
- `BoxCollider`: Box shape for collision

**Creating entities and components:**
```cpp
// Create entity in a scene
se::Entity player = scene->CreateEntity("Player");

// Add components
auto& transform = player.AddComponent<se::TransformComponent>();
transform.Position = {0.0f, 5.0f, 0.0f};

auto& mesh = player.AddComponent<se::MeshRenderComponent>();
mesh.VertexArray = myVertexArray;
mesh.Material = myMaterial;

// Query entities with specific components
auto view = scene->GetAllEntitiesWith<TransformComponent, RigidbodyComponent>();
for (auto entity : view) {
    auto [transform, rb] = view.get<TransformComponent, RigidbodyComponent>(entity);
    // Process entities
}
```

**Important ECS patterns:**
- Components are stored in the `Scene`'s registry, not in `Entity` objects
- `Entity` is just a handle (32-bit ID) + pointer to scene
- Always check `entity.IsValid()` before using entities across frames
- Use `scene->GetAllEntitiesWith<Components...>()` to iterate entities

### Event System

The engine has **two event systems** (in transition):

**Legacy Event System** (`engine/event/`):
- Events inherit from `Event` base class
- Events propagate through layer stack (reverse order)
- Layers handle events via `OnEvent(Event& event)` override
- Event can be marked as handled to stop propagation

**New Event Bus System** (`engine/new_event_system/`):
- Type-safe event channels
- `EventBus` manages listeners and dispatching
- Access via `Application::Get().GetEventBus()`
- Events are queued and dispatched at end of frame

**When adding events**, prefer the new EventBus system for new code.

### Renderer Architecture

The renderer is split into multiple abstraction levels:

**Low-level** (`engine/renderer/`):
- `RenderCommand`: Direct OpenGL command wrapper (Clear, Draw, etc.)
- `VertexArray`, `Buffer`: OpenGL buffer abstractions
- `GraphicsContext`: OpenGL context management

**Mid-level**:
- `Shader`: Shader program loading and uniform management
- `Material`: Shader + uniforms combination
- `Mesh`: VertexArray wrapper (legacy, prefer VertexArray directly)

**High-level**:
- `SceneRenderer`: Manages scene-wide rendering (shadows, lights, submission queue)
- `RenderSystem`: Iterates ECS entities with `MeshRenderComponent` and submits to renderer

**Rendering flow:**
1. `SceneRenderer::BeginScene(camera)` - Set up view/projection matrices
2. Submit geometry: `SceneRenderer::Submit(vertexArray, material, transform)`
3. `SceneRenderer::EndScene()` - Execute shadow pass, then scene pass
4. Or use `Scene::OnRender(camera, aspectRatio)` which automatically renders all `MeshRenderComponent` entities

### Physics System

Physics is powered by Bullet Physics:
- `PhysicsSystem`: Owns `btDiscreteDynamicsWorld`, manages simulation
- `PhysicsWorld`: Wrapper around Bullet world
- `PhysicsManager`: Singleton managing collision shapes and materials

**Physics lifecycle:**
- Created per-scene or shared across scenes
- `Scene::OnUpdate(dt)` automatically steps physics simulation
- Rigidbody entities sync `TransformComponent` after physics update

**Adding physics to an entity:**
```cpp
entity.AddComponent<se::RigidbodyComponent>().Mass = 10.0f;
entity.AddComponent<se::BoxCollider>().HalfExtents = {1.0f, 1.0f, 1.0f};
```

### Input System

Polled input via `InputManager` singleton:
```cpp
#include "engine/input/InputManager.h"

if (se::InputManager::Get().IsKeyDown(se::Key::W)) {
    // Move forward
}

auto [mouseX, mouseY] = se::InputManager::Get().GetMousePosition();
```

The `InputManager` is updated by the application before layers are updated.

## Third-Party Dependencies

Located in `engine/third_party/`:
- **GLFW**: Window and input handling
- **glad**: OpenGL loader
- **glm**: Math library (vectors, matrices, quaternions)
- **entt**: Entity Component System
- **spdlog**: Logging
- **stb**: Image loading (stb_image)
- **ImGui**: Immediate mode GUI
- **ImGuizmo**: 3D gizmos for ImGui
- **imnodes**: Node editor for ImGui
- **Bullet**: Physics engine (BulletDynamics, BulletCollision, LinearMath)
- **RmlUi**: HTML/CSS-like UI library (optional, not heavily used yet)

All dependencies are built as part of the main CMake build (no external package manager).

## Common Patterns and Idioms

### Smart Pointers

The engine uses `std::unique_ptr` and `std::shared_ptr` with custom aliases:
```cpp
se::Scope<T> = std::unique_ptr<T>
se::Ref<T> = std::shared_ptr<T>

// Use helpers:
auto myObj = se::CreateScope<MyClass>(args);
auto shared = se::CreateRef<MyClass>(args);
```

### Logging

Use spdlog-based logging macros:
```cpp
SE_LOG_INFO("Message");
SE_LOG_WARN("Warning: {}", value);
SE_LOG_ERROR("Error occurred");
```

### Math Types

Defined in `Engine.h`:
```cpp
se::Vector2, se::Vector3, se::Vector4  // glm::vec*
se::Matrix4, se::Matrix3, se::Matrix2  // glm::mat*
se::Quaternion  // glm::quat
se::Color  // glm::vec4
```

Rotations in `TransformComponent` are stored as **Euler angles in degrees**, but converted to quaternions for calculations.

## Project Structure

```
Monster-Engine/
├── engine/                    # Engine library
│   ├── include/engine/        # Public headers
│   │   ├── Application.h      # Main application class
│   │   ├── Layer.h            # Layer base class
│   │   ├── Window.h, Renderer.h
│   │   ├── ecs/               # ECS components and systems
│   │   ├── event/             # Legacy event system
│   │   ├── new_event_system/  # New event bus
│   │   ├── input/             # Input management
│   │   ├── physics/           # Physics components and systems
│   │   ├── renderer/          # Renderer abstractions
│   │   └── ui/                # UI systems (ImGui, RmlUi)
│   ├── src/                   # Implementation files
│   ├── third_party/           # External dependencies
│   └── CMakeLists.txt
├── apps/sandbox/              # Example application
│   └── src/                   # Sample layers and demos
├── assets/                    # Shaders, textures, models
│   └── shaders/               # GLSL shader files
├── scripts/                   # Build and utility scripts
├── docs/                      # Documentation
│   ├── Wiki.md                # Developer guides
│   └── Architecture.md        # Architecture diagrams (Mermaid)
└── CMakeLists.txt             # Root build file
```

## Development Workflow

### Adding a New Component

1. Define struct in `engine/include/engine/ecs/Components.h` or new header
2. Components are POD (plain old data) structs
3. No need to register with entt - it's automatic

### Adding a New System

Systems are typically methods on layers or dedicated classes:
```cpp
void MySystem::Update(se::Scene* scene, float dt) {
    auto view = scene->GetAllEntitiesWith<TransformComponent, MyComponent>();
    for (auto entity : view) {
        auto [transform, comp] = view.get<TransformComponent, MyComponent>(entity);
        // Update logic
    }
}
```

### Adding Shaders

1. Place `.vert` and `.frag` files in `assets/shaders/`
2. Shaders are automatically copied to binary directory during build
3. Load shaders at runtime:
```cpp
auto shader = std::make_shared<se::Shader>("assets/shaders/myshader.vert", 
                                           "assets/shaders/myshader.frag");
```

### Working with Scenes

The sandbox typically creates a scene in `OnAttach()`:
```cpp
void MyLayer::OnAttach() {
    scene_ = std::make_unique<se::Scene>("MyScene");
    
    // Create entities, set up scene...
}

void MyLayer::OnUpdate(float dt) {
    scene_->OnUpdate(dt);  // Updates physics, scripts
}

void MyLayer::OnRender() {
    scene_->OnRender(camera_, aspectRatio_);
}
```

## Important Notes

- **Coordinate System**: Right-handed (OpenGL convention). +Y is up, -Z is forward
- **Angles**: Stored as degrees in components, converted to radians for calculations
- **Assets Path**: Use `PROJECT_SOURCE_DIR` macro for asset paths (defined in CMake)
- **Window Coordinates**: Origin (0,0) is top-left for mouse input
- **Debug Builds**: Built in `build/apps/sandbox/Debug/` on Windows, `build/apps/sandbox/` on Unix
- **No Unit Tests**: The project currently has no test framework; testing is done via sandbox examples

## Troubleshooting

**Build fails with "changes-meaning" warning (Mac):**
The root CMakeLists.txt adds `-Wno-changes-meaning` for Unix builds.

**Shaders not found at runtime:**
Ensure working directory is set to project root, or use relative paths from binary location.

**Physics entities not moving:**
Check that scene has physics enabled (`SceneSettings.EnablePhysics = true`) and entities have both `RigidbodyComponent` and a collider.

**ImGui not showing:**
Ensure `ApplicationSpecification.EnableImGui = true` and ImGuiLayer is pushed.
