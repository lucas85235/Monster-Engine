# MonsterEngine

A lightweight, modular 3D game engine written in C++ with a focus on simplicity and architecture.

## Overview

MonsterEngine is designed to be a playground for learning game engine architecture. It features a layered architecture, an Entity Component System (ECS), and a modern OpenGL renderer.

### Key Features
- **Layered Architecture**: Easy to extend with new layers for game logic or tools.
- **ECS (Entity Component System)**: Powered by `entt` for high-performance entity management.
- **Renderer**: Modern OpenGL renderer with shader support, materials, and shadows.
- **Event System**: Robust event bus for decoupling systems.
- **ImGui Integration**: Built-in debug UI support.

## Getting Started

### Prerequisites
- **CMake** (3.10+)
- **C++ Compiler** (C++17 support)
- **GLFW** (Usually handled by submodules or system install)

### Build Instructions

We provide automated scripts to make building easy on all platforms. These scripts are located in the `scripts/` directory.

1. **Clone the repository**:
   ```bash
   git clone --recursive https://github.com/lucas85235/Monster-Engine.git
   cd Monster-Engine
   ```

2. **Build & Run**:

   **Windows**:
   Double-click `scripts\generate_visual_studio_files_and_build.bat` or run in CMD:
   ```cmd
   scripts\generate_visual_studio_files_and_build.bat
   ```

   **Linux and Mac**:
   First, install dependencies (Debian/Ubuntu):
   ```bash
   chmod +x scripts/setup.sh
   ./scripts/setup.sh
   ```
   Then build and run:
   ```bash
   chmod +x scripts/run.sh
   ./scripts/run.sh
   ```

### Running the Sandbox

After building, you can run the `Sandbox` application to see the engine in action:

The `run.sh` script (Linux/Mac) and `generate_visual_studio_files_and_build.bat` (Windows) automatically run the Sandbox after building.

To run manually:

```bash
# Windows
.\build\apps\sandbox\Debug\Sandbox.exe

# Linux/Mac
./build/apps/sandbox/sandbox
```

## Architecture

The engine is built on a few core concepts:

- **Application**: The main entry point that manages the window and main loop.
- **Layers**: Self-contained modules that are updated and rendered in a stack.
- **Scene**: The container for all entities and components.
- **Systems**: Logic that operates on entities with specific components.

For a deep dive into the architecture, including diagrams, check out the [Wiki](docs/Wiki.md) and [Architecture Diagrams](docs/Architecture.md).

## Documentation

- [Wiki](docs/Wiki.md): Detailed guides on creating components, entities, and layers.
- [Architecture](docs/Architecture.md): UML Class and Sequence diagrams.
