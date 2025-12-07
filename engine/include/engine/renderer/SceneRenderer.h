#pragma once

#include <glm.hpp>
#include <memory>
#include <vector>

#include "engine/Camera.h"
#include "engine/Shader.h"
#include "engine/renderer/Material.h"
#include "engine/renderer/VertexArray.h"

namespace se {

struct RenderStats {
    uint32_t DrawCalls     = 0;
    uint32_t TriangleCount = 0;

    void Reset() {
        DrawCalls     = 0;
        TriangleCount = 0;
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
                bool receiveShadows = true);

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

    RenderStats GetStats() const { return stats_; }
    void ResetStats() { stats_.Reset(); }

   private:
    struct Submission {
        std::shared_ptr<VertexArray> vertex_array;
        std::shared_ptr<Material>    material;
        Matrix4                      Transform{1.0f};
        bool                         CastsShadows   = true;
        bool                         ReceiveShadows = true;
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
        bool                 ShadowsEnabled  = true;
        std::vector<Submission> Submissions;
    };

    void InitializeShadowResources();
    void DestroyShadowResources();
    void RenderShadowPass();
    void RenderScenePass();

    SceneData  sceneData_;
    RenderStats stats_;
    bool initialized_ = false;
};

}  // namespace se
