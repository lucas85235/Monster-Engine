#pragma once

#include <glm.hpp>
#include <memory>
#include <vector>

#include "engine/Camera.h"
#include "engine/Shader.h"
#include "engine/renderer/Material.h"
#include "engine/renderer/VertexArray.h"
#include "engine/renderer/OcclusionCuller.h"

namespace se {

struct RenderStats {
    uint32_t DrawCalls       = 0;
    uint32_t TriangleCount   = 0;
    uint32_t TotalObjects    = 0;
    uint32_t FrustumCulled   = 0;
    uint32_t OcclusionCulled = 0;
    uint32_t VisibleObjects  = 0;

    void Reset() {
        DrawCalls       = 0;
        TriangleCount   = 0;
        TotalObjects    = 0;
        FrustumCulled   = 0;
        OcclusionCulled = 0;
        VisibleObjects  = 0;
    }
};

class SceneRenderer {
   public:
    SceneRenderer();
    ~SceneRenderer();

    // Disable copy
    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;

    void Init();
    void Shutdown();

    void BeginScene(const Camera& camera, const Matrix4& projection);
    void EndScene();

    void Submit(const std::shared_ptr<VertexArray>& vertexArray, 
                const std::shared_ptr<Material>& material,
                const Matrix4& transform = Matrix4(1.0f), 
                bool castsShadows = true, 
                bool receiveShadows = true,
                float boundingRadius = 1.0f);

    struct DirectionalLightData {
        Vector3 Direction{0.0f, -1.0f, 0.0f};
        Vector3 Color{1.0f, 1.0f, 1.0f};
        float   Intensity = 1.0f;
        Vector3 Position{0.0f, 0.0f, 0.0f};
        bool    CastShadows = true;
        bool    Active      = false;
    };

    void SetDirectionalLight(const DirectionalLightData& light);
    void ClearDirectionalLight();
    DirectionalLightData GetDirectionalLight() const;

    void SetShadowMapSize(int width, int height);
    void SetShadowDistance(float distance);
    void SetShadowOrthoSize(float size);
    void SetAmbientStrength(float strength);
    void SetAOStrength(float strength);
    void SetAORadius(float radius);

    // Culling settings
    void SetFrustumCullingEnabled(bool enabled) { frustumCullingEnabled_ = enabled; }
    void SetOcclusionCullingEnabled(bool enabled);
    bool IsFrustumCullingEnabled() const { return frustumCullingEnabled_; }
    bool IsOcclusionCullingEnabled() const { return occlusionCullingEnabled_; }

    OcclusionCuller& GetOcclusionCuller() { return occlusionCuller_; }

    RenderStats GetStats() const { return stats_; }
    void ResetStats() { stats_.Reset(); }

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
    };

    struct SceneData {
        Matrix4              ViewMatrix;
        Matrix4              ProjectionMatrix;
        Matrix4              view_projection_matrix;
        DirectionalLightData directional_light;
        Matrix4              LightSpaceMatrix{1.0f};
        glm::ivec2           ShadowMapSize{1024, 1024};
        unsigned int         ShadowFramebuffer  = 0;
        unsigned int         ShadowDepthTexture = 0;
        std::shared_ptr<Shader> ShadowShader;
        float                ShadowDistance  = 100.0f;
        float                ShadowOrthoSize = 10.0f;
        float                AmbientStrength = 0.2f;
        float                AOStrength      = 0.5f;
        float                AORadius        = 1.0f;
        bool                 ShadowsEnabled  = true;
        std::vector<Submission> Submissions;
    };

    void InitializeShadowResources();
    void DestroyShadowResources();
    void RenderShadowPass();
    void RenderScenePass();

    SceneData   sceneData_;
    RenderStats stats_;
    OcclusionCuller occlusionCuller_;
    bool initialized_            = false;
    bool frustumCullingEnabled_  = true;
    bool occlusionCullingEnabled_ = true;
    uint32_t nextObjectId_       = 1;
};

}  // namespace se

