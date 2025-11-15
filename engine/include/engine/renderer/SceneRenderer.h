#pragma once

#include "engine/Camera.h"
#include "engine/renderer/Material.h"
#include "engine/renderer/VertexArray.h"
#include <glm.hpp>
#include <memory>
#include <vector>

namespace se {
    struct RenderStats {
        uint32_t DrawCalls = 0;
        uint32_t TriangleCount = 0;

        void Reset() {
            DrawCalls = 0;
            TriangleCount = 0;
        }
    };

    class SceneRenderer {
    public:
        static void Init();

        static void Shutdown();

        static void BeginScene(const Camera &camera, const glm::mat4 &projection);

        static void EndScene();

        static void Submit(const std::shared_ptr<VertexArray> &vertexArray,
                           const std::shared_ptr<Material> &material,
                           const glm::mat4 &transform = glm::mat4(1.0f), bool castsShadows = true,
                           bool receiveShadows = true);

        struct DirectionalLightData {
            glm::vec3 Direction{0.0f, -1.0f, 0.0f};
            glm::vec3 Color{1.0f, 1.0f, 1.0f};
            float Intensity = 1.0f;
            glm::vec3 Position{0.0f, 0.0f, 0.0f};
            bool CastShadows = true;
            bool Active = false;
        };

        static void SetDirectionalLight(const DirectionalLightData &light);

        static void ClearDirectionalLight();

        static DirectionalLightData GetDirectionalLight();

        static RenderStats GetStats() {
            return stats_;
        }

        static void ResetStats() {
            stats_.Reset();
        }

    private:
        struct Submission {
            std::shared_ptr<VertexArray> vertex_array;
            std::shared_ptr<Material> material;
            glm::mat4 Transform{1.0f};
            bool CastsShadows = true;
            bool ReceiveShadows = true;
        };

        struct SceneData {
            glm::mat4 ViewMatrix;
            glm::mat4 ProjectionMatrix;
            glm::mat4 view_projection_matrix;
            DirectionalLightData directional_light;
            glm::mat4 LightSpaceMatrix{1.0f};
            glm::ivec2 ShadowMapSize{1024, 1024};
            unsigned int ShadowFramebuffer = 0;
            unsigned int ShadowDepthTexture = 0;
            std::shared_ptr<Shader> ShadowShader;
            float ShadowDistance = 100.0f;
            float ShadowOrthoSize = 10.0f;
            float AmbientStrength = 0.2f;
            bool ShadowsEnabled = true;
            std::vector<Submission> Submissions;
        };

        static SceneData *sceneData_;
        static RenderStats stats_;

        static void InitializeShadowResources();

        static void DestroyShadowResources();

        static void RenderShadowPass();

        static void RenderScenePass();
    };
} // namespace se
