#pragma once
/**
 * BlendSpace.h - 1D and 2D blend spaces for animation interpolation.
 * 
 * BlendSpace1D: Velocity-based locomotion blending (Idle → Walk → Run)
 * BlendSpace2D: 8-directional strafe blending with Delaunay triangulation
 */

#include "engine/animation/advanced/Pose.h"
#include "engine/animation/AnimationClip.h"

#include <glm.hpp>
#include <memory>
#include <vector>
#include <array>
#include <string>

namespace se {

class SkinnedModelData;

namespace anim {

struct BlendSample {
    std::shared_ptr<AnimationClip> clip;
    glm::vec2 position{0.0f};
    std::string clipPath;
    
    BlendSample() = default;
    BlendSample(std::shared_ptr<AnimationClip> c, float pos1D) 
        : clip(c), position(pos1D, 0.0f) {}
    BlendSample(std::shared_ptr<AnimationClip> c, glm::vec2 pos2D) 
        : clip(c), position(pos2D) {}
};

struct BlendSpaceData {
    std::string name;
    bool is2D = false;
    std::vector<BlendSample> samples;
    glm::vec2 minBounds{-1.0f, -1.0f};
    glm::vec2 maxBounds{1.0f, 1.0f};
};

class BlendSpace1D {
public:
    BlendSpace1D() = default;
    explicit BlendSpace1D(const std::string& name);
    
    void SetName(const std::string& name) { name_ = name; }
    const std::string& GetName() const { return name_; }
    
    void AddSample(std::shared_ptr<AnimationClip> clip, float position);
    void RemoveSample(size_t index);
    void SetSamplePosition(size_t index, float position);
    void ClearSamples();
    
    void Evaluate(float parameter, Pose& outPose, float time, const SkinnedModelData* skeleton);
    
    float GetAnimationTime(float baseTime) const;
    
    const std::vector<BlendSample>& GetSamples() const { return samples_; }
    size_t GetSampleCount() const { return samples_.size(); }
    
    void SetBounds(float min, float max);
    float GetMinBound() const { return minBound_; }
    float GetMaxBound() const { return maxBound_; }
    
private:
    void SortSamples();
    
    std::string name_;
    std::vector<BlendSample> samples_;
    float minBound_ = 0.0f;
    float maxBound_ = 1.0f;
};

class BlendSpace2D {
public:
    BlendSpace2D() = default;
    explicit BlendSpace2D(const std::string& name);
    
    void SetName(const std::string& name) { name_ = name; }
    const std::string& GetName() const { return name_; }
    
    void AddSample(std::shared_ptr<AnimationClip> clip, glm::vec2 position);
    void RemoveSample(size_t index);
    void SetSamplePosition(size_t index, glm::vec2 position);
    void ClearSamples();
    
    void Triangulate();
    
    void Evaluate(glm::vec2 parameter, Pose& outPose, float time, const SkinnedModelData* skeleton);
    
    const std::vector<BlendSample>& GetSamples() const { return samples_; }
    const std::vector<std::array<int, 3>>& GetTriangles() const { return triangles_; }
    size_t GetSampleCount() const { return samples_.size(); }
    
    void SetBounds(glm::vec2 min, glm::vec2 max);
    glm::vec2 GetMinBounds() const { return minBounds_; }
    glm::vec2 GetMaxBounds() const { return maxBounds_; }
    
    int GetCachedTriangle() const { return cachedTriangle_; }
    
private:
    bool FindTriangle(glm::vec2 point, int& outTriangleIndex, glm::vec3& outBarycentricCoords);
    void ComputeBarycentric(glm::vec2 p, glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec3& out);
    bool PointInTriangle(glm::vec2 p, glm::vec2 a, glm::vec2 b, glm::vec2 c);
    
    std::string name_;
    std::vector<BlendSample> samples_;
    std::vector<std::array<int, 3>> triangles_;
    int cachedTriangle_ = -1;
    glm::vec2 minBounds_{-1.0f, -1.0f};
    glm::vec2 maxBounds_{1.0f, 1.0f};
};

}  // namespace anim
}  // namespace se
