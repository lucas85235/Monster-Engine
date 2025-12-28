#pragma once

#include <glm.hpp>
#include <memory>
#include <cstdint>

namespace se {

class Shader;

namespace gi {

class SparseBrickCache;

enum class RCDebugMode {
    Off = 0,
    ActiveBricks = 1,
    RadianceSlice = 2,
    IrradianceOverlay = 3,
    CascadeLevels = 4,
    ProbePositions = 5
};

struct RCDebugConfig {
    RCDebugMode mode = RCDebugMode::Off;
    int sliceAxis = 2;
    float slicePosition = 0.0f;
    float probeScale = 0.1f;
    float opacity = 0.5f;
    bool showWireframe = true;
    bool showLabels = false;
};

class RCDebugRenderer {
public:
    RCDebugRenderer();
    ~RCDebugRenderer();
    
    void Init();
    void Shutdown();
    
    void SetConfig(const RCDebugConfig& config) { config_ = config; }
    const RCDebugConfig& GetConfig() const { return config_; }
    
    void Render(const glm::mat4& viewProj, 
                const SparseBrickCache* brickCache,
                uint32_t radianceTexture);
    
    void RenderActiveBricks(const glm::mat4& viewProj, 
                            const SparseBrickCache* brickCache);
    
    void RenderRadianceSlice(const glm::mat4& viewProj,
                             uint32_t radianceTexture,
                             int axis, float position);
    
    void RenderIrradianceOverlay(uint32_t irradianceTexture,
                                 int screenWidth, int screenHeight);
    
    void RenderCascadeLevels(const glm::mat4& viewProj,
                             const SparseBrickCache* brickCache);

private:
    void CreateResources();
    void DestroyResources();
    
    glm::vec3 GetCascadeColor(int cascadeLevel) const;
    
    RCDebugConfig config_;
    bool initialized_ = false;
    
    std::shared_ptr<Shader> wireframeShader_;
    std::shared_ptr<Shader> sliceShader_;
    std::shared_ptr<Shader> overlayShader_;
    
    uint32_t cubeVAO_ = 0;
    uint32_t cubeVBO_ = 0;
    uint32_t quadVAO_ = 0;
    uint32_t quadVBO_ = 0;
};

}  // namespace gi
}  // namespace se
