#include "engine/animation/advanced/BlendSpace.h"

#include "engine/resources/ModelData.h"
#include "engine/Log.h"

#include <algorithm>
#include <cmath>

namespace se::anim {

// ==================== BlendSpace1D ====================

BlendSpace1D::BlendSpace1D(const std::string& name) : name_(name) {}

void BlendSpace1D::AddSample(std::shared_ptr<AnimationClip> clip, float position) {
    if (!clip) {
        SE_LOG_WARN("[BlendSpace1D] Cannot add null clip");
        return;
    }
    
    samples_.push_back(BlendSample(clip, position));
    SortSamples();
    
    SE_LOG_INFO("[BlendSpace1D] Added sample '{}' at position {:.2f}", clip->GetName(), position);
}

void BlendSpace1D::RemoveSample(size_t index) {
    if (index >= samples_.size()) {
        return;
    }
    samples_.erase(samples_.begin() + static_cast<ptrdiff_t>(index));
}

void BlendSpace1D::SetSamplePosition(size_t index, float position) {
    if (index >= samples_.size()) {
        return;
    }
    samples_[index].position.x = position;
    SortSamples();
}

void BlendSpace1D::ClearSamples() {
    samples_.clear();
}

void BlendSpace1D::SortSamples() {
    std::sort(samples_.begin(), samples_.end(),
        [](const BlendSample& a, const BlendSample& b) {
            return a.position.x < b.position.x;
        });
}

void BlendSpace1D::SetBounds(float min, float max) {
    minBound_ = min;
    maxBound_ = max;
}

void BlendSpace1D::Evaluate(float parameter, Pose& outPose, float time, const SkinnedModelData* skeleton) {
    if (samples_.empty() || !skeleton) {
        return;
    }
    
    // Clamp parameter to bounds
    parameter = glm::clamp(parameter, minBound_, maxBound_);
    
    // Single sample case
    if (samples_.size() == 1) {
        outPose.SetFromClip(samples_[0].clip.get(), time, skeleton);
        return;
    }
    
    // Find surrounding samples
    size_t lowerIdx = 0;
    size_t upperIdx = samples_.size() - 1;
    
    for (size_t i = 0; i < samples_.size() - 1; ++i) {
        if (samples_[i].position.x <= parameter && samples_[i + 1].position.x >= parameter) {
            lowerIdx = i;
            upperIdx = i + 1;
            break;
        }
    }
    
    // Handle edge cases
    if (parameter <= samples_[0].position.x) {
        outPose.SetFromClip(samples_[0].clip.get(), time, skeleton);
        return;
    }
    
    if (parameter >= samples_.back().position.x) {
        outPose.SetFromClip(samples_.back().clip.get(), time, skeleton);
        return;
    }
    
    // Interpolate between two samples
    float lowerPos = samples_[lowerIdx].position.x;
    float upperPos = samples_[upperIdx].position.x;
    float range = upperPos - lowerPos;
    
    float t = (range > 0.0001f) ? (parameter - lowerPos) / range : 0.0f;
    t = glm::clamp(t, 0.0f, 1.0f);
    
    // Sample poses
    Pose lowerPose(skeleton);
    Pose upperPose(skeleton);
    
    lowerPose.SetFromClip(samples_[lowerIdx].clip.get(), time, skeleton);
    upperPose.SetFromClip(samples_[upperIdx].clip.get(), time, skeleton);
    
    // Blend
    outPose = lowerPose;
    outPose.BlendWith(upperPose, t);
}

// ==================== BlendSpace2D ====================

BlendSpace2D::BlendSpace2D(const std::string& name) : name_(name) {}

void BlendSpace2D::AddSample(std::shared_ptr<AnimationClip> clip, glm::vec2 position) {
    if (!clip) {
        SE_LOG_WARN("[BlendSpace2D] Cannot add null clip");
        return;
    }
    
    samples_.push_back(BlendSample(clip, position));
    Triangulate();
    
    SE_LOG_INFO("[BlendSpace2D] Added sample '{}' at ({:.2f}, {:.2f})", 
                clip->GetName(), position.x, position.y);
}

void BlendSpace2D::RemoveSample(size_t index) {
    if (index >= samples_.size()) {
        return;
    }
    samples_.erase(samples_.begin() + static_cast<ptrdiff_t>(index));
    Triangulate();
}

void BlendSpace2D::SetSamplePosition(size_t index, glm::vec2 position) {
    if (index >= samples_.size()) {
        return;
    }
    samples_[index].position = position;
    Triangulate();
}

void BlendSpace2D::ClearSamples() {
    samples_.clear();
    triangles_.clear();
    cachedTriangle_ = -1;
}

void BlendSpace2D::SetBounds(glm::vec2 min, glm::vec2 max) {
    minBounds_ = min;
    maxBounds_ = max;
}

void BlendSpace2D::Triangulate() {
    triangles_.clear();
    cachedTriangle_ = -1;
    
    if (samples_.size() < 3) {
        return;
    }
    
    // Simple fan triangulation from center for common 4-5 sample case
    // For more complex cases, would need proper Delaunay triangulation
    
    // Find center point (average of all samples)
    glm::vec2 center{0.0f};
    for (const auto& sample : samples_) {
        center += sample.position;
    }
    center /= static_cast<float>(samples_.size());
    
    // Sort samples by angle from center
    std::vector<size_t> sortedIndices(samples_.size());
    for (size_t i = 0; i < samples_.size(); ++i) {
        sortedIndices[i] = i;
    }
    
    std::sort(sortedIndices.begin(), sortedIndices.end(),
        [this, &center](size_t a, size_t b) {
            glm::vec2 da = samples_[a].position - center;
            glm::vec2 db = samples_[b].position - center;
            return std::atan2(da.y, da.x) < std::atan2(db.y, db.x);
        });
    
    // Create triangles using fan from first vertex
    // For 4+ samples, create triangles connecting consecutive samples through center
    if (samples_.size() == 3) {
        triangles_.push_back({
            static_cast<int>(sortedIndices[0]),
            static_cast<int>(sortedIndices[1]),
            static_cast<int>(sortedIndices[2])
        });
    } else if (samples_.size() == 4) {
        // Create 4 triangles (quad split into triangles from corners)
        triangles_.push_back({
            static_cast<int>(sortedIndices[0]),
            static_cast<int>(sortedIndices[1]),
            static_cast<int>(sortedIndices[2])
        });
        triangles_.push_back({
            static_cast<int>(sortedIndices[0]),
            static_cast<int>(sortedIndices[2]),
            static_cast<int>(sortedIndices[3])
        });
    } else {
        // Fan triangulation from first sample for 5+ samples
        for (size_t i = 1; i < samples_.size() - 1; ++i) {
            triangles_.push_back({
                static_cast<int>(sortedIndices[0]),
                static_cast<int>(sortedIndices[i]),
                static_cast<int>(sortedIndices[i + 1])
            });
        }
    }
    
    SE_LOG_INFO("[BlendSpace2D] Triangulated {} samples into {} triangles", 
                samples_.size(), triangles_.size());
}

void BlendSpace2D::ComputeBarycentric(glm::vec2 p, glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec3& out) {
    glm::vec2 v0 = c - a;
    glm::vec2 v1 = b - a;
    glm::vec2 v2 = p - a;
    
    float dot00 = glm::dot(v0, v0);
    float dot01 = glm::dot(v0, v1);
    float dot02 = glm::dot(v0, v2);
    float dot11 = glm::dot(v1, v1);
    float dot12 = glm::dot(v1, v2);
    
    float invDenom = dot00 * dot11 - dot01 * dot01;
    if (std::abs(invDenom) < 0.0001f) {
        out = glm::vec3(1.0f, 0.0f, 0.0f);
        return;
    }
    invDenom = 1.0f / invDenom;
    
    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    
    out.x = 1.0f - u - v;  // Weight for a
    out.y = v;              // Weight for b
    out.z = u;              // Weight for c
}

bool BlendSpace2D::PointInTriangle(glm::vec2 p, glm::vec2 a, glm::vec2 b, glm::vec2 c) {
    glm::vec3 bary;
    ComputeBarycentric(p, a, b, c, bary);
    return (bary.x >= -0.001f && bary.y >= -0.001f && bary.z >= -0.001f);
}

bool BlendSpace2D::FindTriangle(glm::vec2 point, int& outTriangleIndex, glm::vec3& outBarycentricCoords) {
    if (triangles_.empty()) {
        return false;
    }
    
    // Check cached triangle first (temporal coherence)
    if (cachedTriangle_ >= 0 && cachedTriangle_ < static_cast<int>(triangles_.size())) {
        const auto& tri = triangles_[static_cast<size_t>(cachedTriangle_)];
        glm::vec2 a = samples_[static_cast<size_t>(tri[0])].position;
        glm::vec2 b = samples_[static_cast<size_t>(tri[1])].position;
        glm::vec2 c = samples_[static_cast<size_t>(tri[2])].position;
        
        if (PointInTriangle(point, a, b, c)) {
            outTriangleIndex = cachedTriangle_;
            ComputeBarycentric(point, a, b, c, outBarycentricCoords);
            return true;
        }
    }
    
    // Search all triangles
    for (size_t i = 0; i < triangles_.size(); ++i) {
        const auto& tri = triangles_[i];
        glm::vec2 a = samples_[static_cast<size_t>(tri[0])].position;
        glm::vec2 b = samples_[static_cast<size_t>(tri[1])].position;
        glm::vec2 c = samples_[static_cast<size_t>(tri[2])].position;
        
        if (PointInTriangle(point, a, b, c)) {
            outTriangleIndex = static_cast<int>(i);
            cachedTriangle_ = outTriangleIndex;
            ComputeBarycentric(point, a, b, c, outBarycentricCoords);
            return true;
        }
    }
    
    return false;
}

void BlendSpace2D::Evaluate(glm::vec2 parameter, Pose& outPose, float time, const SkinnedModelData* skeleton) {
    if (samples_.empty() || !skeleton) {
        return;
    }
    
    // Clamp parameter to bounds
    parameter = glm::clamp(parameter, minBounds_, maxBounds_);
    
    // Single sample case
    if (samples_.size() == 1) {
        outPose.SetFromClip(samples_[0].clip.get(), time, skeleton);
        return;
    }
    
    // Two samples: linear interpolation
    if (samples_.size() == 2) {
        Pose pose0(skeleton);
        Pose pose1(skeleton);
        pose0.SetFromClip(samples_[0].clip.get(), time, skeleton);
        pose1.SetFromClip(samples_[1].clip.get(), time, skeleton);
        
        // Use distance-based weight
        float d0 = glm::distance(parameter, samples_[0].position);
        float d1 = glm::distance(parameter, samples_[1].position);
        float total = d0 + d1;
        float t = (total > 0.0001f) ? d0 / total : 0.5f;
        
        outPose = pose0;
        outPose.BlendWith(pose1, t);
        return;
    }
    
    // Find containing triangle
    int triangleIdx = -1;
    glm::vec3 baryCoords;
    
    if (!FindTriangle(parameter, triangleIdx, baryCoords)) {
        // Point outside all triangles - find nearest sample
        float minDist = std::numeric_limits<float>::max();
        size_t nearestIdx = 0;
        
        for (size_t i = 0; i < samples_.size(); ++i) {
            float dist = glm::distance(parameter, samples_[i].position);
            if (dist < minDist) {
                minDist = dist;
                nearestIdx = i;
            }
        }
        
        outPose.SetFromClip(samples_[nearestIdx].clip.get(), time, skeleton);
        return;
    }
    
    // Blend using barycentric coordinates
    const auto& tri = triangles_[static_cast<size_t>(triangleIdx)];
    
    Pose pose0(skeleton);
    Pose pose1(skeleton);
    Pose pose2(skeleton);
    
    pose0.SetFromClip(samples_[static_cast<size_t>(tri[0])].clip.get(), time, skeleton);
    pose1.SetFromClip(samples_[static_cast<size_t>(tri[1])].clip.get(), time, skeleton);
    pose2.SetFromClip(samples_[static_cast<size_t>(tri[2])].clip.get(), time, skeleton);
    
    // Normalize barycentric coordinates
    float sum = baryCoords.x + baryCoords.y + baryCoords.z;
    if (sum > 0.0001f) {
        baryCoords /= sum;
    } else {
        baryCoords = glm::vec3(1.0f / 3.0f);
    }
    
    // Three-way blend: first blend pose0 and pose1, then with pose2
    outPose = pose0;
    
    // Blend pose0 (weight bary.x) with pose1 (weight bary.y)
    float t01 = baryCoords.y / (baryCoords.x + baryCoords.y + 0.0001f);
    outPose.BlendWith(pose1, t01);
    
    // Blend result with pose2 (weight bary.z)
    outPose.BlendWith(pose2, baryCoords.z);
}

}  // namespace se::anim
