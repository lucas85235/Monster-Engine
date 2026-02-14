#pragma once

#include <cstdint>
#include <array>
#include <glm.hpp>

class Camera;  // Global namespace

namespace se {

constexpr int CASCADE_COUNT = 4;

struct CascadeData {
    glm::mat4 ViewProjection;
    float SplitDepth = 0.0f;
};

class CascadedShadowMap {
public:
    CascadedShadowMap() = default;
    ~CascadedShadowMap();
    
    void Init(int resolution = 2048);
    void Shutdown();
    bool IsInitialized() const { return initialized_; }
    
    void BeginShadowPass();
    void BeginCascade(int cascadeIndex);
    void EndShadowPass();
    
    void CalculateCascades(const Camera& camera, const glm::vec3& lightDir, float aspectRatio, float maxDistance = 100.0f);
    
    uint32_t GetTextureArray() const { return shadowTextureArray_; }
    const glm::mat4& GetCascadeMatrix(int index) const { return cascadeData_[index].ViewProjection; }
    float GetCascadeSplit(int index) const { return cascadeData_[index].SplitDepth; }
    int GetResolution() const { return resolution_; }
    
    void SetSplitLambda(float lambda) { splitLambda_ = lambda; }
    float GetSplitLambda() const { return splitLambda_; }

private:
    void CalculateSplitDepths(float nearPlane, float farPlane);
    glm::mat4 CalculateLightSpaceMatrix(const Camera& camera, const glm::vec3& lightDir, float aspectRatio, float nearSplit, float farSplit);
    
    uint32_t shadowFBO_ = 0;
    uint32_t shadowTextureArray_ = 0;
    int resolution_ = 2048;
    bool initialized_ = false;
    
    std::array<CascadeData, CASCADE_COUNT> cascadeData_;
    float splitLambda_ = 0.85f;  // Practical split scheme interpolation (0 = uniform, 1 = logarithmic)
};

}  // namespace se
