# Resources Overview

MonsterEngine provides resource management systems for loading and caching assets.

---

## Resource Managers

| Manager | Purpose |
|---------|---------|
| `ModelManager` | Static 3D models |
| `SkinnedModelManager` | Animated models |
| `TextureManager` | Textures |
| `MaterialLibrary` | Materials |
| `AnimationManager` | Animation clips |

---

## Model Loading

```cpp
#include "engine/resources/ModelManager.h"

// Load a static model
auto* manager = se::ModelManager::Get();
auto model = manager->Load("assets/models/building.fbx");

// Create entity from model
auto entity = scene->CreateEntity("Building");
auto& mesh = entity.AddComponent<se::MeshRenderComponent>();
mesh.vertex_array = model->GetMesh(0)->GetVertexArray();
```

### Supported Formats
- FBX
- OBJ
- GLTF / GLB
- DAE (Collada)

---

## Texture Loading

```cpp
#include "engine/resources/TextureManager.h"

auto* textures = se::TextureManager::Get();

// Load texture
auto albedo = textures->Load("assets/textures/brick_albedo.png");
auto normal = textures->Load("assets/textures/brick_normal.png");

// Bind for rendering
albedo->Bind(0);  // Texture unit 0
normal->Bind(1);  // Texture unit 1
```

---

## Shader Loading

```cpp
#include "engine/Shader.h"

auto shader = std::make_shared<se::Shader>();
shader->LoadFromFiles(
    "assets/shaders/pbr.vert",
    "assets/shaders/pbr.frag"
);

// Use shader
shader->Bind();
shader->SetMat4("uModel", modelMatrix);
shader->SetVec3("uCameraPos", cameraPos);
```

---

## Material System

```cpp
#include "engine/resources/MaterialLibrary.h"

// Create material definition
se::MaterialDefinition def;
def.name = "BrickMaterial";
def.shaderPath = "assets/shaders/pbr";
def.AddTexture("albedo", "assets/textures/brick_albedo.png");
def.AddTexture("normal", "assets/textures/brick_normal.png");
def.SetFloat("roughness", 0.8f);
def.SetFloat("metallic", 0.0f);

// Register and get instance
auto* library = se::MaterialLibrary::Get();
library->RegisterDefinition(def);
auto* instance = library->CreateInstance("BrickMaterial");

// Use on entity
mesh.materialInstance = instance;
```

---

## Animation Loading

```cpp
#include "engine/animation/AnimationManager.h"

auto* anims = se::AnimationManager::Get();

// Load animation clip
auto runClip = anims->Load("assets/animations/run.fbx");
auto idleClip = anims->Load("assets/animations/idle.fbx");

// Use with animator
animator->Play(runClip, true);
```

---

## Caching

All managers cache loaded resources:

```cpp
// First call loads from disk
auto tex1 = textures->Load("assets/brick.png");

// Second call returns cached
auto tex2 = textures->Load("assets/brick.png");

// tex1 and tex2 point to same texture
```

---

## See Also

- [Model Loading](ModelLoading.md)
- [Textures](Textures.md)
- [Shaders](Shaders.md)
- [MaterialLibrary](MaterialLibrary.md)
