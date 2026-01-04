# MonsterEngine Wiki

Welcome to the MonsterEngine documentation. This wiki provides comprehensive coverage of the engine's architecture, systems, and how to use them effectively.

---

## Quick Links

| Category | Description |
|----------|-------------|
| [Architecture](Architecture.md) | High-level engine architecture and diagrams |
| [Getting Started](tutorials/GettingStarted.md) | Build your first application |

---

## Core Systems

### [Application & Core](core/Application.md)
The engine entry point, layer system, and application lifecycle.
- [Application](core/Application.md) - Main engine loop and layer management
- [Window](core/Window.md) - GLFW window integration
- [Time](core/Time.md) - Frame timing and delta time
- [Profiling](core/Profiling.md) - Performance profiler and metrics
- [ThreadPool](core/ThreadPool.md) - Async task execution

### [Entity Component System](ecs/Overview.md)
entt-based ECS for game object management.
- [Overview](ecs/Overview.md) - ECS architecture
- [Scene](ecs/Scene.md) - Scene lifecycle and settings
- [Entity](ecs/Entity.md) - Entity creation and management
- [Components](ecs/Components.md) - Built-in components reference
- [Systems](ecs/Systems.md) - RenderSystem, AnimationSystem
- [Scripting](ecs/Scripting.md) - Component-based scripting

---

## Rendering

### [Renderer Overview](renderer/Overview.md)
PBR rendering pipeline with modern graphics techniques.
- [SceneRenderer](renderer/SceneRenderer.md) - Main rendering pipeline
- [Materials](renderer/Materials.md) - PBR material system
- [Shadows](renderer/Shadows.md) - Cascaded Shadow Maps
- [IBL](renderer/IBL.md) - Image-Based Lighting
- [Culling](renderer/Culling.md) - Frustum and occlusion culling
- [Instancing](renderer/Instancing.md) - GPU instanced rendering

### [Post-Processing](postprocess/Overview.md)
Full post-processing pipeline with extensible pass system.
- [Overview](postprocess/Overview.md) - Pipeline architecture
- [Bloom](postprocess/Bloom.md) - HDR bloom effect
- [SSAO](postprocess/SSAO.md) - Screen-space ambient occlusion
- [SSGI](postprocess/SSGI.md) - Screen-space global illumination
- [FXAA](postprocess/FXAA.md) - Fast approximate anti-aliasing
- [Color Grading](postprocess/ColorGrading.md) - Color adjustments
- [Tonemapping](postprocess/Tonemapping.md) - HDR to LDR mapping
- [How to Add New Pass](postprocess/HowToAddNewPass.md) - Extension guide

---

## Game Systems

### [Physics](physics/Overview.md)
Bullet Physics integration for 3D rigid body simulation.
- [Overview](physics/Overview.md) - Physics architecture
- [PhysicsSystem](physics/PhysicsSystem.md) - Configuration and lifecycle
- [Rigidbodies](physics/Rigidbodies.md) - Rigid body components
- [Colliders](physics/Colliders.md) - Collision shapes
- [Raycasting](physics/Raycasting.md) - Ray queries
- [Debug Draw](physics/DebugDraw.md) - Physics visualization

### [Animation](animation/Overview.md)
Skeletal animation with graphs, blend spaces, and IK.
- [Overview](animation/Overview.md) - Animation architecture
- [Animator](animation/Animator.md) - Playback and blending
- [Animation Graph](animation/AnimationGraph.md) - Node-based animation
- [Blend Spaces](animation/BlendSpaces.md) - 1D/2D animation blending
- [Animation Layers](animation/Layers.md) - Partial body blending
- [Locomotion Controller](animation/LocomotionController.md) - Movement animation
- [IK System](animation/IK.md) - Inverse kinematics
- [Bone Attachment](animation/BoneAttachment.md) - Attach objects to bones
- [Procedural Animation](animation/ProceduralAnimation.md) - Look-at and body rotation

### [UI System](ui/Overview.md)
Godot-style retained mode UI framework.
- [Overview](ui/Overview.md) - UI architecture
- [UIControl](ui/UIControl.md) - Base control class
- [Widgets](ui/Widgets.md) - Button, Label, Slider, etc.
- [Containers](ui/Containers.md) - Layout containers
- [Theming](ui/Theming.md) - Visual theming
- [HUD](ui/HUD.md) - HUD controller

---

## Input & Events

### [Input](input/InputManager.md)
Polled input system for keyboard and mouse.
- [InputManager](input/InputManager.md) - Input querying
- [Key Codes](input/KeyCodes.md) - Key code reference

### [Events](events/EventBus.md)
Type-safe event system for decoupled communication.
- [EventBus](events/EventBus.md) - Event dispatching
- [EventChannel](events/EventChannel.md) - Typed channels
- [Built-in Events](events/BuiltInEvents.md) - Window and input events

---

## Resources

### [Resource Management](resources/Overview.md)
Asset loading and caching systems.
- [Model Loading](resources/ModelLoading.md) - 3D model import
- [Textures](resources/Textures.md) - Texture management
- [Shaders](resources/Shaders.md) - Shader system
- [Material Library](resources/MaterialLibrary.md) - Material management
- [Map Loading](resources/MapLoading.md) - Level file format

---

## Tutorials

Step-by-step guides for common tasks:

1. [Getting Started](tutorials/GettingStarted.md) - Create your first app
2. [Creating a Layer](tutorials/CreatingALayer.md) - Game logic layers
3. [Adding Entities](tutorials/AddingEntities.md) - ECS workflow
4. [Custom Components](tutorials/CustomComponents.md) - Extend the ECS
5. [Adding Post-Process Pass](tutorials/AddingPostProcessPass.md) - Extend rendering
6. [Using Physics](tutorials/UsingPhysics.md) - Physics integration

---

## Engine Namespace

All engine code lives under the `se` namespace:

```cpp
#include <Engine.h>

// Create application
se::ApplicationSpecification spec;
spec.Name = "MyGame";
spec.WindowWidth = 1920;
spec.WindowHeight = 1080;

se::Application app(spec);
app.PushLayer<MyGameLayer>();
return app.Run();
```

---

## Version

Current Engine Version: See `ENGINE_VERSION` file.
