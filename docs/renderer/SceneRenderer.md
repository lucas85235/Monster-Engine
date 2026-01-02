# SceneRenderer

The `SceneRenderer` is the central rendering class that manages the complete rendering pipeline from shadow mapping to post-processing.

---

## Overview

SceneRenderer handles:
- PBR lighting with IBL
- Cascaded shadow mapping
- SSAO and SSGI
- GPU frustum and occlusion culling
- Instanced rendering
- Skinned mesh rendering
- Skybox rendering
- Post-processing pipeline

---

## Initialization

```cpp
#include "engine/renderer/SceneRenderer.h"

se::SceneRenderer& renderer = se::Application::Get().GetRenderer().GetSceneRenderer();

// SceneRenderer is automatically initialized by Renderer
// Manual initialization if needed:
renderer.Init();
```

---

## Render Lifecycle

### Frame Structure

```cpp
void MyLayer::OnRender() {
    auto& renderer = se::Application::Get().GetRenderer().GetSceneRenderer();
    
    // 1. Begin frame (binds HDR framebuffer)
    renderer.BeginFrame(windowWidth, windowHeight);
    
    // 2. Begin scene with camera
    renderer.BeginScene(camera_, aspectRatio_);
    
    // 3. Submit objects for rendering
    for (auto& object : objects) {
        renderer.Submit(object.mesh, object.material, object.transform);
    }
    
    // 4. End scene (executes all render passes)
    renderer.EndScene();
    
    // 5. End frame (post-processing & blit to screen)
    renderer.EndFrame();
}
```

### BeginScene

Sets up the camera matrices and lighting:

```cpp
void BeginScene(const Camera& camera, float aspectRatio);
```

### Submit

Queue objects for rendering:

```cpp
// Basic submission
void Submit(
    std::shared_ptr<VertexArray> mesh,
    std::shared_ptr<Material> material,
    const glm::mat4& transform
);

// With PBR parameters
void Submit(
    std::shared_ptr<VertexArray> mesh,
    std::shared_ptr<Material> material,
    const glm::mat4& transform,
    bool castsShadows,
    bool receivesShadows,
    const glm::vec4& color,
    float metallic,
    float roughness,
    float reflectance,
    float ao
);

// Instanced rendering
void SubmitInstanced(
    InstancedMesh* mesh,
    size_t instanceCount
);

// Skinned mesh
void SubmitSkinned(
    SkinnedMesh* mesh,
    const std::vector<glm::mat4>& boneMatrices,
    const glm::mat4& transform
);
```

### EndScene

Executes the complete render pipeline:

```mermaid
graph LR
    A[Shadow Pass] --> B[SSAO]
    B --> C[SSGI]
    C --> D[Main Pass]
    D --> E[Skybox]
    E --> F[Post-Processing]
```

---

## Directional Light

Configure the sun/directional light:

```cpp
se::SceneRenderer::DirectionalLightData light;
light.Direction = glm::normalize(glm::vec3(0.3f, -1.0f, 0.2f));
light.Color = glm::vec3(1.0f, 0.95f, 0.9f);
light.Intensity = 2.0f;
light.CastShadows = true;
light.Active = true;

renderer.SetDirectionalLight(light);
```

---

## Exposure Control

Control HDR exposure:

```cpp
renderer.SetExposure(1.0f);
float exposure = renderer.GetExposure();
```

---

## Shadow Configuration

### Enable/Disable Shadows

```cpp
renderer.SetCSMEnabled(true);
bool enabled = renderer.IsCSMEnabled();
```

### Cascade Visualization

```cpp
renderer.SetVisualizeCascades(true);
// Each cascade rendered in different color
```

### Split Lambda

Controls the logarithmic/uniform split ratio:

```cpp
auto& csm = renderer.GetCascadedShadowMap();
csm.SetSplitLambda(0.85f);  // 0.0 = uniform, 1.0 = logarithmic
```

---

## Culling

### Frustum Culling

```cpp
renderer.SetFrustumCullingEnabled(true);
bool enabled = renderer.IsFrustumCullingEnabled();
```

### Occlusion Culling

```cpp
bool enabled = renderer.IsOcclusionCullingEnabled();
auto* culler = renderer.GetOcclusionCuller();
```

---

## Debug Modes

Visualize different buffers:

```cpp
// Set debug visualization mode
renderer.SetDebugMode(mode);
int currentMode = renderer.GetDebugMode();

// Modes:
// 0 = Off (normal rendering)
// 1 = Ambient Occlusion
// 2 = Normals
// 3 = Roughness  
// 4 = Metallic
```

---

## Global Material Override

For testing, override all materials:

```cpp
se::PBRMaterialParams override;
override.metallic = 0.0f;
override.roughness = 0.5f;

renderer.SetGlobalMaterialOverride(&override);

// Clear override
renderer.ClearGlobalMaterialOverride();
```

---

## Render Statistics

```cpp
se::RenderStats stats = renderer.GetStats();

ImGui::Text("Draw Calls: %u", stats.DrawCalls);
ImGui::Text("Triangles: %u", stats.TriangleCount);
ImGui::Text("Total Objects: %u", stats.TotalObjects);
ImGui::Text("Visible Objects: %u", stats.VisibleObjects);
ImGui::Text("Frustum Culled: %u", stats.FrustumCulled);
ImGui::Text("Occlusion Culled: %u", stats.OcclusionCulled);
ImGui::Text("Instanced Batches: %u", stats.InstancedBatches);
ImGui::Text("Instanced Objects: %u", stats.InstancedObjects);

// Reset stats each frame
renderer.ResetStats();
```

---

## Accessing Subsystems

### SSAO Pass

```cpp
se::SSAOPass* ssao = renderer.GetSSAOPass();
ssao->SetRadius(0.5f);
ssao->SetIntensity(1.5f);
```

### SSGI Pass

```cpp
se::SSGIPass* ssgi = renderer.GetSSGIPass();
ssgi->SetEnabled(true);

se::SSGIConfig& config = ssgi->GetConfig();
config.Intensity = 1.0f;
config.MaxDistance = 20.0f;
```

### Post-Process Pipeline

```cpp
se::PostProcessPipeline* pp = renderer.GetPostProcessPipeline();

// Get specific pass
se::BloomPass* bloom = pp->GetPass<se::BloomPass>();
bloom->SetThreshold(1.0f);
bloom->SetIntensity(0.5f);
```

### IBL Processor

```cpp
se::IBLProcessor* ibl = renderer.GetIBLProcessor();
// IBL textures are automatically bound during rendering
```

---

## Integration with RenderSystem

The `RenderSystem` uses `SceneRenderer` internally:

```cpp
// RenderSystem::RenderScene() does this internally:
void RenderSystem::RenderScene(entt::registry& registry, const Camera& camera, float aspect) {
    auto& renderer = GetSceneRenderer();
    
    renderer.BeginScene(camera, aspect);
    
    auto view = registry.view<TransformComponent, MeshRenderComponent>();
    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        auto& mesh = view.get<MeshRenderComponent>(entity);
        
        if (!mesh.IsVisible) continue;
        
        renderer.Submit(mesh.vertex_array, mesh.material, transform.GetTransform());
    }
    
    renderer.EndScene();
}
```

---

## Example: Complete Render Setup

```cpp
class RenderLayer : public se::Layer {
public:
    void OnAttach() override {
        auto& renderer = se::Application::Get().GetRenderer().GetSceneRenderer();
        
        // Configure directional light
        se::SceneRenderer::DirectionalLightData sun;
        sun.Direction = glm::normalize(glm::vec3(0.5f, -1.0f, 0.3f));
        sun.Color = glm::vec3(1.0f, 0.98f, 0.95f);
        sun.Intensity = 3.0f;
        sun.CastShadows = true;
        sun.Active = true;
        renderer.SetDirectionalLight(sun);
        
        // Configure exposure
        renderer.SetExposure(1.2f);
        
        // Enable features
        renderer.SetCSMEnabled(true);
        renderer.SetFrustumCullingEnabled(true);
        
        // Configure SSAO
        if (auto* ssao = renderer.GetSSAOPass()) {
            ssao->SetRadius(0.6f);
            ssao->SetIntensity(1.5f);
        }
        
        // Configure Bloom
        if (auto* pp = renderer.GetPostProcessPipeline()) {
            if (auto* bloom = pp->GetPass<se::BloomPass>()) {
                bloom->SetThreshold(1.2f);
                bloom->SetIntensity(0.3f);
            }
        }
    }
    
    void OnRender() override {
        scene_->OnRender(*camera_, aspectRatio_);
    }
};
```

---

## See Also

- [Materials](Materials.md)
- [Shadows](Shadows.md)
- [Post-Processing](../postprocess/Overview.md)
- [SSAO](../postprocess/SSAO.md)
- [SSGI](../postprocess/SSGI.md)
