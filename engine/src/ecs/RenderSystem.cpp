#include "engine/ecs/RenderSystem.h"

#include <filesystem>

#include "engine/Log.h"
#include "engine/core/ServiceLocator.h"
#include "engine/ecs/ModelComponent.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/renderer/Material.h"
#include "engine/renderer/SceneRenderer.h"
#include "engine/resources/MaterialManager.h"
#include "engine/resources/Model.h"

namespace se {

bool                             RenderSystem::initialized_ = false;
RenderSystem::InstanceBatchMap   RenderSystem::instanceBatches_;
RenderSystem::InstancedMeshCache RenderSystem::instancedMeshCache_;
uint32_t                         RenderSystem::lastBatchCount_       = 0;
uint32_t                         RenderSystem::lastInstancedObjects_ = 0;
std::shared_ptr<Material>        RenderSystem::instancedMaterial_    = nullptr;
std::shared_ptr<Material>        RenderSystem::modelMaterial_        = nullptr;

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

    if (!fs::exists(vertPath) || !fs::exists(fragPath)) {
        SE_LOG_ERROR("Instanced shaders not found at: {}", vertPath.string());
        return;
    }

    auto shader = MaterialManager::GetShader("InstancedShader", vertPath, fragPath);
    if (!shader) {
        SE_LOG_ERROR("Failed to load instanced shader");
        return;
    }

    instancedMaterial_ = MaterialManager::CreateMaterial(shader);
    if (!instancedMaterial_) {
        SE_LOG_ERROR("Failed to create instanced material");
        return;
    }
    instancedMaterial_->SetFloat("uSpecularStrength", 0.5f);
    SE_LOG_INFO("Created instanced material with shader: {}", vertPath.string());
}

void RenderSystem::EnsureModelMaterial() {
    if (modelMaterial_) return;

    namespace fs = std::filesystem;

    fs::path assetsPath = fs::current_path() / "assets";
    if (!fs::exists(assetsPath)) {
        SE_LOG_ERROR("Cannot find assets folder at: {}", assetsPath.string());
        return;
    }

    fs::path vertPath = assetsPath / "shaders" / "simple.vert";
    fs::path fragPath = assetsPath / "shaders" / "simple.frag";

    if (!fs::exists(vertPath) || !fs::exists(fragPath)) {
        SE_LOG_ERROR("Simple shaders not found at: {}", vertPath.string());
        return;
    }

    auto shader = MaterialManager::GetShader("ModelShader", vertPath, fragPath);
    if (!shader) {
        SE_LOG_ERROR("Failed to load model shader");
        return;
    }

    modelMaterial_ = MaterialManager::CreateMaterial(shader);
    if (!modelMaterial_) {
        SE_LOG_ERROR("Failed to create model material");
        return;
    }
    
    SE_LOG_INFO("Created model material with shader: {}", vertPath.string());
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
    modelMaterial_.reset();
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
    auto lightView = scene.GetAllEntitiesWith<TransformComponent, DirectionalLightComponent>();
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
    SE_LOG_INFO("DEBUG: BeginScene starting");
    glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
    sceneRenderer.BeginScene(camera, projection);
    SE_LOG_INFO("DEBUG: BeginScene complete");

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

        if (!meshRender.vertex_array || !meshRender.material) {
            skippedCount++;
            continue;
        }

        InstanceBatchKey key{meshRender.vertex_array.get(), meshRender.material.get()};

        InstanceData instanceData;
        instanceData.Transform = transform.GetTransform();
        instanceData.Color     = meshRender.Color;  // Use per-entity color

        instanceBatches_[key].push_back(instanceData);
    }

    // Process entities with ModelComponent (3D models loaded from files)
    SE_LOG_INFO("DEBUG: Starting ModelComponent loop");
    auto modelView = scene.GetAllEntitiesWith<TransformComponent, ModelComponent>();
    int modelCount = 0;
    for (auto entity : modelView) {
        auto& transform = modelView.get<TransformComponent>(entity);
        auto& modelComp = modelView.get<ModelComponent>(entity);

        if (!modelComp.IsVisible || !modelComp.model) {
            skippedCount++;
            continue;
        }

        modelCount++;
        SE_LOG_INFO("RenderSystem: Rendering model with {} submeshes", 
                    modelComp.model->GetSubMeshCount());

        // Render each submesh of the model
        int submeshIndex = 0;
        for (const auto& submesh : modelComp.model->GetSubMeshes()) {
            auto va = submesh.GetVertexArray();
            if (!va) {
                SE_LOG_WARN("RenderSystem: Submesh {} has no vertex array", submeshIndex);
                submeshIndex++;
                continue;
            }

            SE_LOG_INFO("RenderSystem: Submesh {} has VA with {} indices", 
                        submeshIndex, va->GetIndexBuffer() ? va->GetIndexBuffer()->GetCount() : 0);

            // Use model material (non-instanced shader with uModel uniform)
            auto material = submesh.GetMaterial();
            if (!material) {
                EnsureModelMaterial();
                material = modelMaterial_;
            }

            if (!material) {
                SE_LOG_ERROR("RenderSystem: No material available for submesh {}", submeshIndex);
                submeshIndex++;
                continue;
            }

            SE_LOG_INFO("RenderSystem: Submitting submesh {} to scene renderer", submeshIndex);

            // Submit directly to scene renderer (no instancing for now)
            sceneRenderer.Submit(va, material, transform.GetTransform(), 
                                 modelComp.CastShadows, modelComp.ReceiveShadows);
            
            SE_LOG_INFO("RenderSystem: Submesh {} submitted successfully", submeshIndex);
            submeshIndex++;
        }
    }
    
    if (modelCount > 0) {
        SE_LOG_INFO("RenderSystem: Processed {} models", modelCount);
    }

    SE_LOG_INFO("DEBUG: Starting batch processing");

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
            if (meshRender.vertex_array.get() == key.va && meshRender.material.get() == key.mat) {
                material = meshRender.material;
                va       = meshRender.vertex_array;
                break;
            }
        }

        if (!material || !va) continue;

        if (instances.size() == 1) {
            // Single instance - use normal submit for simplicity
            sceneRenderer.Submit(va, material, instances[0].Transform, true, true);
        } else {
            // Multiple instances - use instanced rendering
            instancedObjects += static_cast<uint32_t>(instances.size());

            // Get or create InstancedMesh for this batch
            auto it = instancedMeshCache_.find(key);
            if (it == instancedMeshCache_.end()) {
                // Create new InstancedMesh with capacity for growth
                uint32_t maxInstances =
                    std::max(static_cast<uint32_t>(instances.size() * 2), 1000u);
                auto instancedMesh = std::make_shared<InstancedMesh>(va, maxInstances);
                it                 = instancedMeshCache_.emplace(key, instancedMesh).first;
                SE_LOG_INFO("Created InstancedMesh for batch with capacity {}", maxInstances);
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
            SE_LOG_INFO("DEBUG: About to SetInstances");
            instancedMesh->SetInstances(instances);
            SE_LOG_INFO("DEBUG: SetInstances complete");

            // Ensure we have the instanced material loaded
            EnsureInstancedMaterial();

            // Submit to SceneRenderer for instanced rendering (use instanced material for proper
            // shader)
            if (instancedMaterial_) {
                sceneRenderer.SubmitInstanced(instancedMesh, instancedMaterial_, true, true);
            }
            SE_LOG_INFO("DEBUG: Batch submitted");
        }
    }
    SE_LOG_INFO("DEBUG: All batches processed");

    lastBatchCount_       = batchCount;
    lastInstancedObjects_ = instancedObjects;

    // Log stats periodically
    static int frameCount = 0;
    if (frameCount < 10 || frameCount % 300 == 0) {
        SE_LOG_INFO("RenderSystem: {} batches, {} instanced objects, {} skipped", batchCount,
                    instancedObjects, skippedCount);
    }
    frameCount++;

    SE_LOG_INFO("DEBUG: About to call EndScene");
    sceneRenderer.EndScene();
    SE_LOG_INFO("DEBUG: EndScene complete");
}

}  // namespace se
