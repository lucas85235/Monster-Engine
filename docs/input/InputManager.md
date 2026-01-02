# InputManager

The `InputManager` provides polled input for keyboard and mouse state.

---

## Overview

Query input state at any time:

```cpp
#include "engine/input/InputManager.h"

auto& input = se::InputManager::Get();

// Keyboard
if (input.IsKeyDown(se::Key::W)) {
    // W is held
}

// Mouse
glm::vec2 mousePos = input.GetMousePosition();
if (input.IsMouseButtonDown(se::Mouse::ButtonLeft)) {
    // Left click held
}
```

---

## Keyboard Input

### Key State

```cpp
bool held = input.IsKeyDown(se::Key::Space);
```

### Common Keys

```cpp
// Movement
se::Key::W, se::Key::A, se::Key::S, se::Key::D
se::Key::Up, se::Key::Down, se::Key::Left, se::Key::Right

// Actions
se::Key::Space, se::Key::LeftShift, se::Key::LeftControl
se::Key::Enter, se::Key::Escape, se::Key::Tab

// Numbers
se::Key::D0, se::Key::D1, ..., se::Key::D9

// Function keys
se::Key::F1, se::Key::F2, ..., se::Key::F12
```

See [KeyCodes](KeyCodes.md) for complete list.

---

## Mouse Input

### Position

```cpp
glm::vec2 pos = input.GetMousePosition();
float x = input.GetMouseX();
float y = input.GetMouseY();
```

### Buttons

```cpp
if (input.IsMouseButtonDown(se::Mouse::ButtonLeft)) { }
if (input.IsMouseButtonDown(se::Mouse::ButtonRight)) { }
if (input.IsMouseButtonDown(se::Mouse::ButtonMiddle)) { }
```

---

## Example: Camera Control

```cpp
void UpdateCamera(float dt) {
    auto& input = se::InputManager::Get();
    
    // WASD movement
    glm::vec3 move{0.0f};
    if (input.IsKeyDown(se::Key::W)) move.z -= 1.0f;
    if (input.IsKeyDown(se::Key::S)) move.z += 1.0f;
    if (input.IsKeyDown(se::Key::A)) move.x -= 1.0f;
    if (input.IsKeyDown(se::Key::D)) move.x += 1.0f;
    
    // Sprint
    float speed = input.IsKeyDown(se::Key::LeftShift) ? 10.0f : 5.0f;
    
    camera.Move(move * speed * dt);
    
    // Mouse look (when right button held)
    if (input.IsMouseButtonDown(se::Mouse::ButtonRight)) {
        glm::vec2 delta = input.GetMouseDelta();
        camera.Rotate(delta.x * 0.1f, delta.y * 0.1f);
    }
}
```

---

## API Reference

### Keyboard

| Method | Description |
|--------|-------------|
| `IsKeyDown(Key)` | Is key currently pressed |

### Mouse

| Method | Description |
|--------|-------------|
| `IsMouseButtonDown(Mouse)` | Is button currently pressed |
| `GetMousePosition()` | Get cursor position (vec2) |
| `GetMouseX()` | Get cursor X position |
| `GetMouseY()` | Get cursor Y position |

### System

| Method | Description |
|--------|-------------|
| `Get()` | Get singleton instance |
| `Update()` | Called internally each frame |

---

## See Also

- [KeyCodes](KeyCodes.md)
- [Event System](../events/EventBus.md)
