#include "engine/ecs/RenderSystem.h"

#include <filesystem>

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/core/ServiceLocator.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/renderer/Material.h"
#include "engine/renderer/SceneRenderer.h"
#include "engine/resources/MaterialManager.h"

namespace se {

bool                             RenderSystem::initialized_ = false;
RenderSystem::InstanceBatchMap   RenderSystem::instanceBatches_;
RenderSystem::InstancedMeshCache RenderSystem::instancedMeshCache_;
uint32_t                         RenderSystem::lastBatchCount_       = 0;
uint32_t                         RenderSystem::lastInstancedObjects_ = 0;
std::shared_ptr<Material>        RenderSystem::instancedMaterial_    = nullptr;

void RenderSystem::EnsureInstancedMaterial() {
    if (instancedMaterial_) return;

    namespace fs = std::filesystem;

    // Try to find assets folder relative to current working directory
    fs::path assetsPath = fs::current_path() / "assets";
    if (!fs::exists(assetsPath)) {
        SE_LOG_ERROR("Cannot find assets folder at: {}", assetsPath.string());
        return;
    }

    fs::path vertPath = assetsPath / "shaders" / "instanced.vert";
    fs::path fragPath = assetsPath / "shaders" / "instanced.frag";

    // Check removed: loading handled by Shader::CreateFromFiles (supports SPIR-V)
    // if (!fs::exists(vertPath) || !fs::exists(fragPath)) {
    //     SE_LOG_ERROR("Instanced shaders not found at: {}", vertPath.string());
    //     return;
    // }

    auto shader = MaterialManager::GetShader("InstancedShader", vertPath, fragPath);
    if (!shader) {
        SE_LOG_ERROR("Failed to load instanced shader");
        return;
    }

    instancedMaterial_ = MaterialManager::CreateMaterial(shader);
    instancedMaterial_->SetFloat("uSpecularStrength", 0.5f);

    // Assign White Texture to Instanced Material to ensure untextured objects render white (not black)
    // uTexture is bound to slot 0 in shader.
    auto whiteTex = MaterialManager::GetWhiteTexture();
    if (whiteTex) { instancedMaterial_->SetTexture("uTexture", whiteTex->GetHandle()); }

    SE_LOG_INFO("Created instanced material with shader: {}", vertPath.string());
}

void RenderSystem::Init() {
    if (initialized_) {
        SE_LOG_WARN("RenderSystem already initialized");
        return;
    }

    SE_LOG_INFO("Initializing RenderSystem with automatic instancing");
    initialized_ = true;
}

void RenderSystem::Shutdown() {
    if (!initialized_) return;

    SE_LOG_INFO("Shutting down RenderSystem");
    instanceBatches_.clear();
    instancedMeshCache_.clear();
    instancedMaterial_.reset();
    initialized_ = false;
}

void RenderSystem::Render(Scene& scene, const Camera& camera, float aspectRatio) {
    if (!initialized_) {
        SE_LOG_ERROR("RenderSystem not initialized!");
        return;
    }

    if (!ServiceLocator::Get().HasSceneRenderer()) {
        SE_LOG_ERROR("SceneRenderer not available in ServiceLocator!");
        return;
    }
    SceneRenderer& sceneRenderer = ServiceLocator::Get().GetSceneRenderer();

    // Configure lighting
    sceneRenderer.ClearDirectionalLight();
    SceneRenderer::DirectionalLightData lightData;
    auto                                lightView = scene.GetAllEntitiesWith<TransformComponent, DirectionalLightComponent>();
    for (auto entity : lightView) {
        auto& transform = lightView.get<TransformComponent>(entity);
        auto& light     = lightView.get<DirectionalLightComponent>(entity);

        if (!light.Enabled) continue;

        glm::vec3 direction = -transform.GetForward();
        if (glm::length(direction) <= 0.0f) { direction = glm::vec3(0.0f, -1.0f, 0.0f); }

        lightData.Direction   = glm::normalize(direction);
        lightData.Position    = transform.Position;
        lightData.Color       = light.Color;
        lightData.Intensity   = glm::max(light.Intensity, 0.0f);
        lightData.CastShadows = light.CastShadows;
        lightData.Active      = true;
        sceneRenderer.SetDirectionalLight(lightData);
        break;
    }

    // Begin scene rendering
    glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
    sceneRenderer.BeginScene(camera, projection);

    // Clear batches from previous frame (reuse containers to avoid allocations)
    for (auto& [key, instances] : instanceBatches_) { instances.clear(); }

    // Get all entities with TransformComponent and MeshRenderComponent
    auto view = scene.GetAllEntitiesWith<TransformComponent, MeshRenderComponent>();

    int skippedCount = 0;

    // Group entities by (VertexArray, Material) for instancing
    for (auto entity : view) {
        auto& transform  = view.get<TransformComponent>(entity);
        auto& meshRender = view.get<MeshRenderComponent>(entity);

        if (!meshRender.IsVisible) {
            skippedCount++;
            continue;
        }

        if (!meshRender.MeshVertexArray || !meshRender.MeshMaterial) {
            skippedCount++;
            continue;
        }

        InstanceBatchKey key{meshRender.MeshVertexArray.get(), meshRender.MeshMaterial.get(), meshRender.BoundingRadius};

        InstanceData instanceData;
        instanceData.Transform = transform.GetTransform();
        instanceData.Color     = meshRender.Color;  // Use per-entity color

        instanceBatches_[key].push_back(instanceData);
    }

    // Process batches
    uint32_t batchCount       = 0;
    uint32_t instancedObjects = 0;

    for (auto& [key, instances] : instanceBatches_) {
        if (instances.empty()) continue;

        batchCount++;

        // Find original material from the key
        std::shared_ptr<Material>    material = nullptr;
        std::shared_ptr<VertexArray> va       = nullptr;

        // Search for matching material in entities (we need shared_ptr)
        for (auto entity : view) {
            auto& meshRender = view.get<MeshRenderComponent>(entity);
            if (meshRender.MeshVertexArray.get() == key.va && meshRender.MeshMaterial.get() == key.mat) {
                material = meshRender.MeshMaterial;
                va       = meshRender.MeshVertexArray;
                break;
            }
        }

        if (!material || !va) continue;

        // Check if we should use instanced rendering
        // Vulkan instanced pipeline not fully implemented yet - use individual submits
        bool useInstancing = (instances.size() > 1);
        if (Application::Get().GetGraphicsAPI() == RHI::API::Vulkan) {
            useInstancing = false;  // Force individual draws for Vulkan until pipeline instancing works
        }

        if (!useInstancing) {
            // Individual submit for each instance
            static int logCount = 0;
            if (logCount < 5) {
                SE_LOG_INFO("RenderSystem: Submitting {} instances individually for batch (Vulkan fallback)", instances.size());
                logCount++;
            }
            for (const auto& inst : instances) {
                sceneRenderer.Submit(va, material, inst.Transform, true, true, key.boundingRadius);
            }
        } else {
            // Multiple instances - use instanced rendering (OpenGL path)
            instancedObjects += static_cast<uint32_t>(instances.size());

            // Get or create InstancedMesh for this batch
            auto it = instancedMeshCache_.find(key);
            if (it == instancedMeshCache_.end()) {
                // Create new InstancedMesh with capacity for growth
                uint32_t maxInstances  = std::max(static_cast<uint32_t>(instances.size() * 2), 1000u);
                auto     instancedMesh = std::make_shared<InstancedMesh>(va, maxInstances);
                it                     = instancedMeshCache_.emplace(key, instancedMesh).first;
                SE_LOG_INFO("Created InstancedMesh for batch with capacity {} (API={}, should not happen for Vulkan!)", 
                    maxInstances, (int)Application::Get().GetGraphicsAPI());
            }

            auto& instancedMesh = it->second;

            // Check if we need to resize
            if (instances.size() > instancedMesh->GetMaxInstances()) {
                uint32_t newMax = static_cast<uint32_t>(instances.size() * 2);
                instancedMesh   = std::make_shared<InstancedMesh>(va, newMax);
                it->second      = instancedMesh;
                SE_LOG_INFO("Resized InstancedMesh to capacity {}", newMax);
            }

            // Upload instance data and draw
            instancedMesh->SetInstances(instances);

            // Ensure we have the instanced material loaded
            EnsureInstancedMaterial();

            // Submit to SceneRenderer for instanced rendering (use instanced material for proper shader)
            if (instancedMaterial_) { sceneRenderer.SubmitInstanced(instancedMesh, instancedMaterial_, true, true); }
        }
    }

    lastBatchCount_       = batchCount;
    lastInstancedObjects_ = instancedObjects;

    // Log stats periodically (disabled for cleaner output)
    // static int frameCount = 0;
    // if (frameCount < 10 || frameCount % 300 == 0) {
    //     SE_LOG_INFO("RenderSystem: {} batches, {} instanced objects, {} skipped", batchCount, instancedObjects, skippedCount);
    // }
    // frameCount++;

    sceneRenderer.EndScene();
}

}  // namespace se
