#include "engine/ecs/RenderSystem.h"

#include <filesystem>
#include <glad/glad.h>
#include "engine/Log.h"
#include "engine/core/ServiceLocator.h"
#include "engine/debug/FrameProfiler.h"
#include "engine/ecs/AnimatorComponent.h"
#include "engine/ecs/ModelComponent.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/ecs/SkinnedModelComponent.h"
#include "engine/renderer/Material.h"
#include "engine/renderer/MaterialInstance.h"
#include "engine/renderer/PBRMaterial.h"
#include "engine/renderer/SceneRenderer.h"
#include "engine/renderer/Texture.h"
#include "engine/renderer/TextureMaterial.h"
#include "engine/resources/MaterialManager.h"
#include "engine/resources/Model.h"
#include "engine/resources/ModelData.h"

namespace se {

bool                             RenderSystem::initialized_ = false;
RenderSystem::InstanceBatchMap   RenderSystem::instanceBatches_;
RenderSystem::InstancedMeshCache RenderSystem::instancedMeshCache_;
uint32_t                         RenderSystem::lastBatchCount_       = 0;
uint32_t                         RenderSystem::lastInstancedObjects_ = 0;
std::shared_ptr<Material>        RenderSystem::instancedMaterial_    = nullptr;
std::shared_ptr<Material>        RenderSystem::modelMaterial_        = nullptr;
std::shared_ptr<Material>        RenderSystem::skinnedMaterial_      = nullptr;
std::array<int, RenderSystem::MAX_BONES> RenderSystem::boneUniformLocations_ = {};
bool                             RenderSystem::boneLocationsInitialized_ = false;
uint32_t                         RenderSystem::boneUniformShaderID_ = 0;
std::unordered_map<InstanceBatchKey, size_t, InstanceBatchKeyHash> RenderSystem::lastFrameInstanceCounts_;
RenderSystem::BatchResourcesCache RenderSystem::batchResources_;

void RenderSystem::InitBoneUniformLocations(Shader* shader) {
    if (!shader) return;
    
    // Re-initialize if shader changed (e.g., after resize/recreation)
    uint32_t currentShaderID = shader->getID();
    if (boneLocationsInitialized_ && currentShaderID == boneUniformShaderID_) {
        return;
    }
    
    char uniformName[64];
    for (size_t i = 0; i < MAX_BONES; ++i) {
        snprintf(uniformName, sizeof(uniformName), "uBoneMatrices[%zu]", i);
        boneUniformLocations_[i] = shader->getUniformLocation(uniformName);
    }
    boneUniformShaderID_ = currentShaderID;
    boneLocationsInitialized_ = true;
}

void RenderSystem::EnsureInstancedMaterial() {
    if (instancedMaterial_) return;

    namespace fs = std::filesystem;

    // Try to find assets folder relative to current working directory
    fs::path assetsPath = fs::current_path() / "assets";
    if (!fs::exists(assetsPath)) {
        SE_LOG_ERROR("Cannot find assets folder at: {}", assetsPath.string());
        return;
    }

    fs::path vertPath = assetsPath / "shaders" / "core" / "instanced.vert";
    fs::path fragPath = assetsPath / "shaders" / "core" / "instanced.frag";

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

    fs::path vertPath = assetsPath / "shaders" / "core" / "model.vert";
    fs::path fragPath = assetsPath / "shaders" / "core" / "model.frag";

    if (!fs::exists(vertPath) || !fs::exists(fragPath)) {
        SE_LOG_ERROR("Model shaders not found at: {}", vertPath.string());
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

void RenderSystem::EnsureSkinnedMaterial() {
    if (skinnedMaterial_) return;

    namespace fs = std::filesystem;

    fs::path assetsPath = fs::current_path() / "assets";
    if (!fs::exists(assetsPath)) {
        SE_LOG_ERROR("Cannot find assets folder at: {}", assetsPath.string());
        return;
    }

    fs::path vertPath = assetsPath / "shaders" / "core" / "skinned_model.vert";
    fs::path fragPath = assetsPath / "shaders" / "core" / "skinned_model.frag";

    if (!fs::exists(vertPath) || !fs::exists(fragPath)) {
        SE_LOG_ERROR("Skinned model shaders not found at: {}", vertPath.string());
        return;
    }

    auto shader = MaterialManager::GetShader("SkinnedModelShader", vertPath, fragPath);
    if (!shader) {
        SE_LOG_ERROR("Failed to load skinned model shader");
        return;
    }

    skinnedMaterial_ = MaterialManager::CreateMaterial(shader);
    if (!skinnedMaterial_) {
        SE_LOG_ERROR("Failed to create skinned model material");
        return;
    }
    
    SE_LOG_INFO("Created skinned model material with shader: {}", vertPath.string());
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

    // Configure lighting from scene entities (only if entities exist)
    auto lightView = scene.GetAllEntitiesWith<TransformComponent, DirectionalLightComponent>();
    for (auto entity : lightView) {
        auto& transform = lightView.get<TransformComponent>(entity);
        auto& light     = lightView.get<DirectionalLightComponent>(entity);

        if (!light.Enabled) continue;

        glm::vec3 direction = -transform.GetForward();
        if (glm::length(direction) <= 0.0f) { direction = glm::vec3(0.0f, -1.0f, 0.0f); }

        SceneRenderer::DirectionalLightData lightData;
        lightData.Direction   = glm::normalize(direction);
        lightData.Position    = transform.Position;
        lightData.Color       = light.Color;
        lightData.Intensity   = glm::max(light.Intensity, 0.0f);
        lightData.CastShadows = light.CastShadows;
        lightData.Active      = true;
        sceneRenderer.SetDirectionalLight(lightData);
        break;  // Only use first enabled light
    }

    // Begin scene rendering
    glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
    sceneRenderer.BeginScene(camera, projection);

    for (auto& [key, instances] : instanceBatches_) {
        // Reserve based on last frame's count to minimize reallocations
        auto lastIt = lastFrameInstanceCounts_.find(key);
        if (lastIt != lastFrameInstanceCounts_.end() && lastIt->second > 0) {
            instances.reserve(lastIt->second);
        }
        instances.clear();
    }
    
    // Clear batch resources cache (will be re-populated during grouping)
    batchResources_.clear();

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

        // Emissive objects must be rendered individually to pass their emissive properties
        if (meshRender.EmissiveFactor > 0.0f) {
            sceneRenderer.Submit(meshRender.vertex_array, meshRender.material, 
                                 transform.WorldMatrix, true, true, 1.0f, nullptr,
                                 meshRender.EmissiveColor, meshRender.EmissiveFactor);
            continue;
        }
        
        // Use MaterialInstance if available (new unified material system)
        if (meshRender.materialInstance) {
            auto shader = meshRender.material ? meshRender.material->GetShader() : nullptr;
            if (shader) {
                shader->bind();
                meshRender.materialInstance->Bind(shader.get());
            }
            sceneRenderer.Submit(meshRender.vertex_array, meshRender.material, 
                                 transform.WorldMatrix, meshRender.CastShadows, 
                                 meshRender.ReceiveShadows, 1.0f, nullptr);
            continue;
        }
        
        // Legacy: Custom PBR objects must be rendered individually to pass their PBR params
        if (meshRender.UseCustomPBR) {
            // Create a temporary PBR override for this object
            PBRMaterialParams pbrParams;
            pbrParams.BaseColor = meshRender.Color;
            pbrParams.Metallic = meshRender.Metallic;
            pbrParams.Roughness = meshRender.Roughness;
            pbrParams.Reflectance = meshRender.Reflectance;
            pbrParams.AO = meshRender.AO;
            pbrParams.EmissiveColor = meshRender.EmissiveColor;
            pbrParams.EmissiveFactor = meshRender.EmissiveFactor;
            
            sceneRenderer.SubmitWithPBR(meshRender.vertex_array, meshRender.material, 
                                        transform.WorldMatrix, pbrParams,
                                        meshRender.CastShadows, meshRender.ReceiveShadows,
                                        meshRender.customTextureMaterial);
            continue;
        }

        InstanceBatchKey key{meshRender.vertex_array.get(), meshRender.material.get()};

        // Cache shared_ptrs and properties on first encounter for this batch
        if (batchResources_.find(key) == batchResources_.end()) {
            batchResources_[key] = BatchResources{
                meshRender.vertex_array,
                meshRender.material,
                meshRender.EmissiveColor,
                meshRender.EmissiveFactor
            };
        }

        InstanceData instanceData;
        instanceData.Transform = transform.WorldMatrix;
        instanceData.Color     = meshRender.Color;  // Use per-entity color

        instanceBatches_[key].push_back(instanceData);
    }

    // Process entities with ModelComponent (3D models loaded from files)
    auto modelView = scene.GetAllEntitiesWith<TransformComponent, ModelComponent>();
    for (auto entity : modelView) {
        auto& transform = modelView.get<TransformComponent>(entity);
        auto& modelComp = modelView.get<ModelComponent>(entity);

        if (!modelComp.IsVisible || !modelComp.model) {
            skippedCount++;
            continue;
        }

        // Check if this entity also has an AnimatorComponent for skeletal animation
        bool hasAnimator = scene.GetRegistry().all_of<AnimatorComponent>(entity);
        AnimatorComponent* animComp = nullptr;
        if (hasAnimator) {
            animComp = &scene.GetRegistry().get<AnimatorComponent>(entity);
        }

        // Render each submesh of the model
        for (const auto& submesh : modelComp.model->GetSubMeshes()) {
            auto va = submesh.GetVertexArray();
            if (!va) continue;

            // Choose shader based on whether we have animation
            std::shared_ptr<Material> material;
            if (hasAnimator && animComp && animComp->HasBones()) {
                EnsureSkinnedMaterial();
                material = skinnedMaterial_;
            } else {
                material = submesh.GetMaterial();
                if (!material) {
                    EnsureModelMaterial();
                    material = modelMaterial_;
                }
            }

            if (!material) continue;

            // Bind textures from TextureMaterial if present
            auto texMat = submesh.GetTextureMaterial();
            auto shader = material->GetShader();
            
            // Debug logging for texture/material status (once per frame at start)
            static int debugFrameCount = 0;
            bool shouldLog = (debugFrameCount < 5) || (debugFrameCount % 300 == 0);
            
            if (shader) {
                shader->bind();
                
                // Set bone matrices if animating
                if (hasAnimator && animComp && animComp->HasBones()) {
                    const auto& boneMatrices = animComp->GetBoneMatrices();
                    shader->setInt("uHasBones", boneMatrices.empty() ? 0 : 1);
                    
                    // Initialize cached uniform locations once
                    InitBoneUniformLocations(shader.get());
                    
                    // Use cached locations for fast bone matrix setting
                    for (size_t i = 0; i < boneMatrices.size() && i < MAX_BONES; ++i) {
                        shader->setMat4ByLocation(boneUniformLocations_[i], boneMatrices[i]);
                    }
                    
                    if (shouldLog) {
                        SE_LOG_INFO("RenderSystem: Rendering with {} bone matrices", boneMatrices.size());
                    }
                } else {
                    shader->setInt("uHasBones", 0);
                }
                
                // Use MaterialInstance if available (new unified system)
                auto matInstance = submesh.GetMaterialInstance();
                if (matInstance) {
                    matInstance->Bind(shader.get());
                    if (shouldLog) {
                        SE_LOG_INFO("RenderSystem: Using MaterialInstance for submesh '{}'", submesh.GetName());
                    }
                } else {
                    // Fallback to legacy TextureMaterial binding
                    int hasAlbedo = 0, hasNormal = 0, hasSpecular = 0, hasAO = 0;
                    
                    if (texMat) {
                        if (texMat->HasAlbedo()) {
                            texMat->Albedo->Bind(1);
                            hasAlbedo = 1;
                        }
                        if (texMat->HasNormal()) {
                            texMat->Normal->Bind(2);
                            hasNormal = 1;
                        }
                        if (texMat->HasSpecular()) {
                            texMat->Specular->Bind(3);
                            hasSpecular = 1;
                        }
                        if (texMat->HasAO()) {
                            texMat->AO->Bind(4);
                            hasAO = 1;
                        }
                    }
                    
                    shader->setInt("uAlbedoMap", 1);
                    shader->setInt("uNormalMap", 2);
                    shader->setInt("uSpecularMap", 3);
                    shader->setInt("uAOMap", 4);
                    
                    shader->setInt("uHasAlbedo", hasAlbedo);
                    shader->setInt("uHasNormal", hasNormal);
                    shader->setInt("uHasSpecular", hasSpecular);
                    shader->setInt("uHasAO", hasAO);
                    
                    shader->setVec4("uBaseColor", texMat ? texMat->BaseColor : glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
                    shader->setFloat("uShininess", texMat ? texMat->Shininess : 32.0f);
                    
                    if (shouldLog && texMat) {
                        SE_LOG_INFO("RenderSystem PBR: submesh='{}' animated={} albedo={} normal={} spec={} ao={}",
                                    submesh.GetName(), hasAnimator, hasAlbedo, hasNormal, hasSpecular, hasAO);
                    }
                }
            }
            
            debugFrameCount++;

            // Submit directly to scene renderer with TextureMaterial for PBR
            sceneRenderer.Submit(va, material, transform.WorldMatrix, 
                                 modelComp.CastShadows, modelComp.ReceiveShadows, 1.0f, texMat);
        }
    }

    // Process entities with SkinnedModelComponent (animated models with bone data in vertices)
    auto skinnedView = scene.GetAllEntitiesWith<TransformComponent, SkinnedModelComponent>();
    for (auto entity : skinnedView) {
        auto& transform = skinnedView.get<TransformComponent>(entity);
        auto& skinnedComp = skinnedView.get<SkinnedModelComponent>(entity);

        if (!skinnedComp.IsVisible || !skinnedComp.model) {
            skippedCount++;
            continue;
        }

        // Check for AnimatorComponent for bone matrices
        AnimatorComponent* animComp = nullptr;
        if (scene.GetRegistry().all_of<AnimatorComponent>(entity)) {
            animComp = &scene.GetRegistry().get<AnimatorComponent>(entity);
        }

        // Get skinned shader
        EnsureSkinnedMaterial();
        if (!skinnedMaterial_) continue;

        auto shader = skinnedMaterial_->GetShader();
        if (!shader) continue;

        shader->bind();

        // Set view/projection matrices
        shader->setMat4("uView", camera.getViewMatrix());
        shader->setMat4("uProj", projection);

        // Set bone matrices
        bool hasBones = animComp && animComp->HasBones();
        shader->setInt("uHasBones", hasBones ? 1 : 0);
        
        if (hasBones) {
            const auto& boneMatrices = animComp->GetBoneMatrices();
            
            // Initialize cached uniform locations once
            InitBoneUniformLocations(shader.get());
            
            // Use cached locations for fast bone matrix setting
            for (size_t i = 0; i < boneMatrices.size() && i < MAX_BONES; ++i) {
                shader->setMat4ByLocation(boneUniformLocations_[i], boneMatrices[i]);
            }
        }

        // Set model matrix
        shader->setMat4("uModel", transform.WorldMatrix);

        // Set lighting uniforms
        auto light = sceneRenderer.GetDirectionalLight();
        shader->setVec3("uLightDirection", -light.Direction);
        shader->setVec3("uLightColor", light.Color);
        shader->setFloat("uLightIntensity", light.Active ? light.Intensity : 0.0f);
        shader->setFloat("uAmbientStrength", 0.3f);
        shader->setFloat("uReceiveShadows", skinnedComp.ReceiveShadows ? 1.0f : 0.0f);
        shader->setFloat("uShadowsEnabled", sceneRenderer.IsShadowsEnabled() ? 1.0f : 0.0f);
        shader->setFloat("uAOStrength", 0.5f);
        shader->setFloat("uAORadius", 1.0f);
        
        // Bind shadow map texture (from previous frame for receive shadows)
        shader->setMat4("uLightSpaceMatrix", sceneRenderer.GetLightSpaceMatrix());
        shader->setInt("uShadowMap", 0);
        glActiveTexture(GL_TEXTURE0);
        uint32_t shadowTex = sceneRenderer.GetShadowDepthTexture();
        if (shadowTex != 0) {
            glBindTexture(GL_TEXTURE_2D, shadowTex);
        }
        
        // Set IBL uniforms (get from SceneRenderer's IBL data)
        const auto& iblData = sceneRenderer.GetEnvironmentLighting();
        shader->setVec3Array("uSH", iblData.SphericalHarmonics, 9);
        shader->setFloat("uIBLIntensity", iblData.Intensity);
        shader->setVec3("uSkyColor", iblData.SkyColor);
        shader->setVec3("uGroundColor", iblData.GroundColor);
        shader->setFloat("uExposure", 1.0f);

        // Set default texture uniforms
        shader->setInt("uHasAlbedo", 0);
        shader->setInt("uHasNormal", 0);
        shader->setInt("uHasSpecular", 0);
        shader->setInt("uHasAO", 0);
        shader->setInt("uHasMetallic", 0);
        shader->setInt("uHasRoughness", 0);
        shader->setInt("uHasEmissive", 0);
        shader->setInt("uHasMetallicRoughness", 0);
        shader->setVec4("uBaseColor", glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
        shader->setFloat("uMetallicFactor", 0.0f);
        shader->setFloat("uRoughnessFactor", 0.5f);
        shader->setFloat("uReflectance", 0.5f);
        shader->setFloat("uAOFactor", 1.0f);
        shader->setFloat("uNormalScale", 1.0f);
        shader->setFloat("uShininess", 32.0f);

        // Draw all meshes
        for (const auto& mesh : skinnedComp.model->GetMeshes()) {
            auto texMat = mesh.GetMaterial();
            
            // Use MaterialInstance if available (new unified system)
            auto matInstance = mesh.GetMaterialInstance();
            if (matInstance) {
                matInstance->Bind(shader.get());
            } else {
                // Fallback to legacy TextureMaterial binding
                int hasAlbedo = 0, hasNormal = 0, hasSpecular = 0, hasAO = 0;
                int hasRoughness = 0, hasMetallic = 0, hasEmissive = 0;
                
                if (texMat) {
                    if (texMat->HasAlbedo()) {
                        texMat->Albedo->Bind(1);
                        hasAlbedo = 1;
                    }
                    if (texMat->HasNormal()) {
                        texMat->Normal->Bind(2);
                        hasNormal = 1;
                    }
                    if (texMat->HasSpecular()) {
                        texMat->Specular->Bind(3);
                        hasSpecular = 1;
                    }
                    if (texMat->HasAO()) {
                        texMat->AO->Bind(4);
                        hasAO = 1;
                    }
                    if (texMat->HasRoughness()) {
                        texMat->Roughness->Bind(5);
                        hasRoughness = 1;
                    }
                    if (texMat->HasMetallic()) {
                        texMat->Metallic->Bind(6);
                        hasMetallic = 1;
                    }
                    if (texMat->HasEmissive()) {
                        texMat->Emissive->Bind(7);
                        hasEmissive = 1;
                    }
                    
                    shader->setVec4("uBaseColor", texMat->BaseColor);
                    shader->setFloat("uMetallicFactor", texMat->MetallicFactor);
                    shader->setFloat("uRoughnessFactor", texMat->RoughnessFactor);
                    shader->setVec3("uEmissiveColor", texMat->EmissiveColor);
                }
                
                // Set sampler uniform locations
                shader->setInt("uAlbedoMap", 1);
                shader->setInt("uNormalMap", 2);
                shader->setInt("uSpecularMap", 3);
                shader->setInt("uAOMap", 4);
                shader->setInt("uRoughnessMap", 5);
                shader->setInt("uMetallicMap", 6);
                shader->setInt("uEmissiveMap", 7);
                
                // Set texture presence flags
                shader->setInt("uHasAlbedo", hasAlbedo);
                shader->setInt("uHasNormal", hasNormal);
                shader->setInt("uHasSpecular", hasSpecular);
                shader->setInt("uHasAO", hasAO);
                shader->setInt("uHasRoughness", hasRoughness);
                shader->setInt("uHasMetallic", hasMetallic);
                shader->setInt("uHasEmissive", hasEmissive);
            }
            
            mesh.Draw();
            
            // Submit for shadow casting
            if (skinnedComp.CastShadows) {
                std::vector<glm::mat4> bones;
                if (hasBones) {
                    bones = animComp->GetBoneMatrices();
                }
                sceneRenderer.SubmitSkinnedForShadow(
                    mesh.GetVaoId(), mesh.GetIndexCount(), 
                    transform.WorldMatrix, bones, hasBones);
            }
        }
        
        // Debug log
        static int skinnedDebugCount = 0;
        if (skinnedDebugCount < 5 || skinnedDebugCount % 300 == 0) {
            glm::vec3 worldPos = glm::vec3(transform.WorldMatrix[3]);
            SE_LOG_INFO("RenderSystem: Drew SkinnedModel '{}' at ({:.2f},{:.2f},{:.2f}), {} meshes, hasBones={}",
                        skinnedComp.model->GetName(), worldPos.x, worldPos.y, worldPos.z,
                        skinnedComp.model->GetMeshCount(), hasBones);
        }
        skinnedDebugCount++;
    }

    // Process batches
    uint32_t batchCount       = 0;
    uint32_t instancedObjects = 0;

    for (auto& [key, instances] : instanceBatches_) {
        if (instances.empty()) continue;

        batchCount++;

        // Use cached resources instead of linear search (O(1) lookup)
        auto resourceIt = batchResources_.find(key);
        if (resourceIt == batchResources_.end()) continue;
        
        const auto& resources = resourceIt->second;
        const auto& va = resources.va;
        const auto& material = resources.material;

        if (!material || !va) continue;

        if (instances.size() == 1) {
            // Single instance - use normal submit with emissive properties
            sceneRenderer.Submit(va, material, instances[0].Transform, true, true, 1.0f, nullptr, 
                                 resources.emissiveColor, resources.emissiveFactor);
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
            instancedMesh->SetInstances(instances);

            // Ensure we have the instanced material loaded
            EnsureInstancedMaterial();

            // Submit to SceneRenderer for instanced rendering (use instanced material for proper
            // shader)
            if (instancedMaterial_) {
                sceneRenderer.SubmitInstanced(instancedMesh, instancedMaterial_, true, true);
            }
        }
    }

    lastBatchCount_       = batchCount;
    lastInstancedObjects_ = instancedObjects;

    // Log stats periodically
    static int frameCount = 0;
    if (frameCount < 10 || frameCount % 300 == 0) {
        SE_LOG_INFO("RenderSystem: {} batches, {} instanced objects, {} skipped", batchCount,
                    instancedObjects, skippedCount);
    }
    frameCount++;
    
    // Track instance counts for next frame's reserve() optimization
    for (const auto& [key, instances] : instanceBatches_) {
        lastFrameInstanceCounts_[key] = instances.size();
    }

    sceneRenderer.EndScene();
}

}  // namespace se
