#pragma once

#include <glm.hpp>
#include <memory>
#include <vector>

#include "engine/Camera.h"
#include "engine/Shader.h"
#include "engine/renderer/GBufferPass.h"
#include "engine/renderer/InstancedMesh.h"
#include "engine/renderer/Material.h"
#include "engine/renderer/OcclusionCuller.h"
#include "engine/renderer/PBRMaterial.h"
#include "engine/renderer/RadianceCascadesPass.h"
#include "engine/renderer/SceneVoxelizer.h"
#include "engine/renderer/SparseRadianceCascades.h"
#include "engine/renderer/SSGIPass.h"
#include "engine/renderer/CascadedShadowMap.h"
#include "engine/renderer/VertexArray.h"

namespace se {

// Forward declaration
struct TextureMaterial;

struct RenderStats {
    uint32_t DrawCalls        = 0;
    uint32_t TriangleCount    = 0;
    uint32_t TotalObjects     = 0;
    uint32_t FrustumCulled    = 0;
    uint32_t OcclusionCulled  = 0;
    uint32_t VisibleObjects   = 0;
    uint32_t InstancedBatches = 0;
    uint32_t InstancedObjects = 0;

    void Reset() {
        DrawCalls        = 0;
        TriangleCount    = 0;
        TotalObjects     = 0;
        FrustumCulled    = 0;
        OcclusionCulled  = 0;
        VisibleObjects   = 0;
        InstancedBatches = 0;
        InstancedObjects = 0;
    }
};

class SceneRenderer {
   public:
    SceneRenderer();
    ~SceneRenderer();

    // Disable copy
    SceneRenderer(const SceneRenderer&)            = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;

    void Init();
    void Shutdown();

    void BeginScene(const Camera& camera, const Matrix4& projection);
    void EndScene();

    void Submit(const std::shared_ptr<VertexArray>& vertexArray,
                const std::shared_ptr<Material>& material, const Matrix4& transform = Matrix4(1.0f),
                bool castsShadows = true, bool receiveShadows = true, float boundingRadius = 1.0f,
                const std::shared_ptr<TextureMaterial>& textureMaterial = nullptr,
                const Vector3& emissiveColor = Vector3(0.0f), float emissiveFactor = 0.0f);

    // Submit instanced geometry (multiple transforms in a single draw call)
    void SubmitInstanced(const std::shared_ptr<InstancedMesh>& instancedMesh,
                         const std::shared_ptr<Material>& material, bool castsShadows = true,
                         bool receiveShadows = true,
                         const Vector3& emissiveColor = Vector3(0.0f), float emissiveFactor = 0.0f);

    // Submit skinned model for shadow casting only (uses raw VAO since SkinnedMesh doesn't use VertexArray)
    void SubmitSkinnedForShadow(
        uint32_t vaoId,
        uint32_t indexCount,
        const Matrix4& transform,
        const std::vector<Matrix4>& boneMatrices,
        bool hasBones);


    struct DirectionalLightData {
        Vector3 Direction{0.0f, -1.0f, 0.0f};
        Vector3 Color{1.0f, 1.0f, 1.0f};
        float   Intensity = 1.0f;
        Vector3 Position{0.0f, 0.0f, 0.0f};
        bool    CastShadows = true;
        bool    Active      = false;
    };

    void                 SetDirectionalLight(const DirectionalLightData& light);
    void                 ClearDirectionalLight();
    DirectionalLightData GetDirectionalLight() const;

    void SetShadowMapSize(int width, int height);
    void SetShadowDistance(float distance);
    void SetShadowOrthoSize(float size);
    void SetAmbientStrength(float strength);
    void SetAOStrength(float strength);
    void SetAORadius(float radius);
    
    // PBR Environment lighting
    void SetEnvironmentLighting(const IBLData& ibl);
    const IBLData& GetEnvironmentLighting() const { return iblData_; }
    
    // HDR Exposure control
    void SetExposure(float exposure) { sceneData_.Exposure = exposure; }
    float GetExposure() const { return sceneData_.Exposure; }
    
    // Radiance Cascades (Global Illumination) - 2D Screen-Space
    void SetRadianceCascadesEnabled(bool enabled);
    bool IsRadianceCascadesEnabled() const;
    RadianceCascadeConfig& GetRadianceCascadeConfig();
    const RadianceCascadeConfig& GetRadianceCascadeConfig() const;
    RadianceCascadesPass* GetRadianceCascadesPass() { return radianceCascades_.get(); }
    
    // Sparse Radiance Cascades (Global Illumination) - 3D World-Space
    void SetSparseRCEnabled(bool enabled);
    bool IsSparseRCEnabled() const;
    SparseRCConfig& GetSparseRCConfig();
    SparseRadianceCascades* GetSparseRadianceCascades() { return sparseRC_.get(); }
    SceneVoxelizer* GetSceneVoxelizer() { return voxelizer_.get(); }
    
    // SSGI (Screen Space Global Illumination) - HBIL
    void SetSSGIEnabled(bool enabled);
    bool IsSSGIEnabled() const;
    SSGIConfig& GetSSGIConfig();
    const SSGIConfig& GetSSGIConfig() const;
    SSGIPass* GetSSGIPass() { return ssgiPass_.get(); }
    
    // Screen size management (needed for GBuffer and RC)
    void SetScreenSize(int width, int height);

    // Culling settings
    void SetFrustumCullingEnabled(bool enabled) {
        frustumCullingEnabled_ = enabled;
    }
    void SetOcclusionCullingEnabled(bool enabled);
    bool IsFrustumCullingEnabled() const {
        return frustumCullingEnabled_;
    }
    bool IsOcclusionCullingEnabled() const {
        return occlusionCullingEnabled_;
    }

    OcclusionCuller& GetOcclusionCuller() {
        return occlusionCuller_;
    }

    RenderStats GetStats() const {
        return stats_;
    }
    void ResetStats() {
        stats_.Reset();
    }
    
    // Global PBR material override for testing
    void SetGlobalMaterialOverride(PBRMaterialParams* params) {
        globalMaterialOverride_ = params;
    }
    void ClearGlobalMaterialOverride() {
        globalMaterialOverride_ = nullptr;
    }
    PBRMaterialParams* GetGlobalMaterialOverride() const {
        return globalMaterialOverride_;
    }
    
    // Cascaded Shadow Maps control
    void SetCSMEnabled(bool enabled) { csmEnabled_ = enabled; }
    bool IsCSMEnabled() const { return csmEnabled_; }
    void SetVisualizeCascades(bool enabled) { visualizeCascades_ = enabled; }
    bool IsVisualizeCascadesEnabled() const { return visualizeCascades_; }
    void SetCSMSplitLambda(float lambda);

   private:
    struct Submission {
        std::shared_ptr<VertexArray> vertex_array;
        std::shared_ptr<Material>    material;
        Matrix4                      Transform{1.0f};
        bool                         CastsShadows   = true;
        bool                         ReceiveShadows = true;
        float                        BoundingRadius = 1.0f;
        Vector3                      Center{0.0f};
        uint32_t                     ObjectId = 0;
        std::shared_ptr<TextureMaterial> textureMaterial;  // PBR texture data
        Vector3                      EmissiveColor{0.0f, 0.0f, 0.0f};  // Emissive for GI
        float                        EmissiveFactor = 0.0f;             // Emission intensity
    };

    struct SceneData {
        Matrix4                 ViewMatrix;
        Matrix4                 ProjectionMatrix;
        Matrix4                 view_projection_matrix;
        DirectionalLightData    directional_light;
        Matrix4                 LightSpaceMatrix{1.0f};
        glm::ivec2              ShadowMapSize{1024, 1024};
        unsigned int            ShadowFramebuffer  = 0;
        unsigned int            ShadowDepthTexture = 0;
        std::shared_ptr<Shader> ShadowShader;
        std::shared_ptr<Shader> InstancedShadowShader;
        std::shared_ptr<Shader> SkinnedShadowShader;
        float                   ShadowDistance  = 100.0f;
        float                   ShadowOrthoSize = 10.0f;
        float                   AmbientStrength = 0.2f;
        float                   AOStrength      = 0.5f;
        float                   AORadius        = 1.0f;
        float                   Exposure        = 1.5f;  // HDR exposure (>1 brighter)
        bool                    ShadowsEnabled  = true;
        Vector3                 CameraPosition{0.0f};    // For GI occlusion
        const Camera*           CurrentCamera = nullptr; // For CSM
        std::vector<Submission> Submissions;
    };
    
    IBLData iblData_;

    // Instanced submission for batched rendering
    struct InstancedSubmission {
        std::shared_ptr<InstancedMesh> instancedMesh;
        std::shared_ptr<Material>      material;
        bool                           castsShadows   = true;
        bool                           receiveShadows = true;
        glm::vec3                      EmissiveColor = glm::vec3(0.0f);
        float                          EmissiveFactor = 0.0f;
    };

    std::vector<InstancedSubmission> instancedSubmissions_;

    // Skinned model submission for shadow casting
    struct SkinnedSubmission {
        uint32_t                     vaoId = 0;
        uint32_t                     indexCount = 0;
        Matrix4                      transform{1.0f};
        std::vector<Matrix4>         boneMatrices;
        bool                         hasBones = false;
    };
    std::vector<SkinnedSubmission> skinnedSubmissions_;

    void InitializeShadowResources();
    void DestroyShadowResources();
    void RenderShadowPass();
    void RenderGBufferPass();  // Renders scene to G-Buffer for deferred lighting
    void RenderScenePass();

    SceneData       sceneData_;
    RenderStats     stats_;
    OcclusionCuller occlusionCuller_;
    bool            initialized_             = false;
    bool            frustumCullingEnabled_   = true;
    bool            occlusionCullingEnabled_ = true;
    uint32_t        nextObjectId_            = 1;
    
    // Radiance Cascades (Global Illumination) - 2D Screen-Space
    std::unique_ptr<RadianceCascadesPass> radianceCascades_;
    
    // Sparse Radiance Cascades (Global Illumination) - 3D World-Space
    std::shared_ptr<SceneVoxelizer> voxelizer_;
    std::unique_ptr<SparseRadianceCascades> sparseRC_;
    
    // G-Buffer for deferred lighting
    std::unique_ptr<GBufferPass> gbuffer_;
    std::shared_ptr<Shader> gbufferShader_;
    std::shared_ptr<Shader> gbufferInstancedShader_;
    
    // SSGI (Screen Space Global Illumination)
    std::unique_ptr<SSGIPass> ssgiPass_;
    
    // Screen dimensions for RC initialization
    int screenWidth_ = 1280;
    int screenHeight_ = 720;
    
    // Global PBR material override for testing
    PBRMaterialParams* globalMaterialOverride_ = nullptr;
    
    // Skybox rendering
    std::shared_ptr<Shader> skyboxShader_;
    uint32_t skyboxVAO_ = 0;
    uint32_t skyboxVBO_ = 0;
    bool skyboxInitialized_ = false;
    
    void InitSkybox();
    void RenderSkybox();
    
    // Cascaded Shadow Maps
    std::unique_ptr<CascadedShadowMap> csm_;
    bool csmEnabled_ = true;
    bool visualizeCascades_ = false;
    void RenderCSMPass();
};

}  // namespace se
