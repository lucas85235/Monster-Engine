#pragma once

#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <string>

namespace se {

class SkinnedModelData;

namespace anim {

class Pose;

struct IKTarget {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    float weight = 1.0f;
    bool useRotation = false;
};

class IIKSolver {
public:
    virtual ~IIKSolver() = default;
    
    virtual void Initialize(const SkinnedModelData* skeleton) = 0;
    
    virtual void Solve(Pose& pose, const IKTarget& target) = 0;
    
    virtual std::string GetName() const = 0;
    
    virtual void SetEnabled(bool enabled) { enabled_ = enabled; }
    virtual bool IsEnabled() const { return enabled_; }
    
protected:
    bool enabled_ = true;
};

}  // namespace anim
}  // namespace se
