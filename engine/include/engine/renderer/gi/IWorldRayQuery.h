#pragma once

#include <glm.hpp>
#include <memory>

namespace se {
namespace gi {

struct RayHit {
    bool hit = false;
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec3 albedo{0.5f};
    glm::vec3 emissive{0.0f};
    float distance = 0.0f;
    uint32_t materialId = 0;
};

struct DirectLightSample {
    glm::vec3 radiance{0.0f};
    float visibility = 1.0f;
};

class IWorldRayQuery {
public:
    virtual ~IWorldRayQuery() = default;
    
    virtual RayHit TraceSegment(const glm::vec3& origin, 
                                 const glm::vec3& direction,
                                 float tMin, 
                                 float tMax) = 0;
    
    virtual DirectLightSample SampleDirectLighting(const glm::vec3& position,
                                                    const glm::vec3& normal) = 0;
    
    virtual bool IsReady() const = 0;
    
    virtual void Update() = 0;
};

}  // namespace gi
}  // namespace se
