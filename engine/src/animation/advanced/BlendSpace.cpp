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
    
    // Try to detect if samples form a regular grid
    // Collect unique X and Y coordinates
    std::vector<float> uniqueX, uniqueY;
    const float epsilon = 0.01f;
    
    for (const auto& sample : samples_) {
        bool foundX = false, foundY = false;
        for (float x : uniqueX) {
            if (std::abs(x - sample.position.x) < epsilon) {
                foundX = true;
                break;
            }
        }
        if (!foundX) uniqueX.push_back(sample.position.x);
        
        for (float y : uniqueY) {
            if (std::abs(y - sample.position.y) < epsilon) {
                foundY = true;
                break;
            }
        }
        if (!foundY) uniqueY.push_back(sample.position.y);
    }
    
    // Sort coordinates
    std::sort(uniqueX.begin(), uniqueX.end());
    std::sort(uniqueY.begin(), uniqueY.end());
    
    size_t cols = uniqueX.size();
    size_t rows = uniqueY.size();
    
    // Check if samples form a complete grid
    if (cols >= 2 && rows >= 2 && cols * rows == samples_.size()) {
        SE_LOG_INFO("[BlendSpace2D] Detected {}x{} grid, using grid triangulation", cols, rows);
        
        // Build a lookup table: grid[row][col] = sample index
        std::vector<std::vector<int>> grid(rows, std::vector<int>(cols, -1));
        
        for (size_t i = 0; i < samples_.size(); ++i) {
            const auto& pos = samples_[i].position;
            
            // Find column
            int col = -1;
            for (size_t c = 0; c < cols; ++c) {
                if (std::abs(uniqueX[c] - pos.x) < epsilon) {
                    col = static_cast<int>(c);
                    break;
                }
            }
            
            // Find row
            int row = -1;
            for (size_t r = 0; r < rows; ++r) {
                if (std::abs(uniqueY[r] - pos.y) < epsilon) {
                    row = static_cast<int>(r);
                    break;
                }
            }
            
            if (col >= 0 && row >= 0) {
                grid[row][col] = static_cast<int>(i);
            }
        }
        
        // Create triangles for each cell (2 triangles per cell)
        for (size_t r = 0; r < rows - 1; ++r) {
            for (size_t c = 0; c < cols - 1; ++c) {
                int tl = grid[r + 1][c];     // top-left
                int tr = grid[r + 1][c + 1]; // top-right
                int bl = grid[r][c];         // bottom-left
                int br = grid[r][c + 1];     // bottom-right
                
                if (tl >= 0 && tr >= 0 && bl >= 0 && br >= 0) {
                    // First triangle: bottom-left, top-left, top-right
                    triangles_.push_back({bl, tl, tr});
                    // Second triangle: bottom-left, top-right, bottom-right
                    triangles_.push_back({bl, tr, br});
                }
            }
        }
        
        SE_LOG_INFO("[BlendSpace2D] Created {} triangles for grid", triangles_.size());
        return;
    }
    
    // Fallback: center fan triangulation for non-grid layouts
    SE_LOG_INFO("[BlendSpace2D] Using fan triangulation (samples don't form a complete grid)");
    
    glm::vec2 center{0.0f};
    for (const auto& sample : samples_) {
        center += sample.position;
    }
    center /= static_cast<float>(samples_.size());
    
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
    
    if (samples_.size() == 3) {
        triangles_.push_back({
            static_cast<int>(sortedIndices[0]),
            static_cast<int>(sortedIndices[1]),
            static_cast<int>(sortedIndices[2])
        });
    } else if (samples_.size() == 4) {
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
    
    // Single sample case - normalized time keeps looping correct
    if (samples_.size() == 1) {
        auto* clip = samples_[0].clip.get();
        float refDuration = clip ? clip->GetDuration() : 24.0f;
        float refTicksPerSecond = clip ? clip->GetTicksPerSecond() : 24.0f;
        if (refTicksPerSecond <= 0.0f) refTicksPerSecond = 24.0f;
        if (refDuration <= 0.0f) refDuration = 24.0f;
        float timeInTicks = time * refTicksPerSecond;
        float normalizedTime = fmod(timeInTicks / refDuration, 1.0f);
        if (normalizedTime < 0.0f) normalizedTime += 1.0f;
        
        outPose.SetFromClipNormalized(clip, normalizedTime, skeleton);
        return;
    }
    
    // Two samples: linear interpolation with normalized time
    if (samples_.size() == 2) {
        // Calculate normalized time from first sample
        auto* refClip = samples_[0].clip.get();
        float refDuration = refClip ? refClip->GetDuration() : 24.0f;
        float refTicksPerSecond = refClip ? refClip->GetTicksPerSecond() : 24.0f;
        if (refTicksPerSecond <= 0.0f) refTicksPerSecond = 24.0f;
        if (refDuration <= 0.0f) refDuration = 24.0f;
        float timeInTicks = time * refTicksPerSecond;
        float normalizedTime = fmod(timeInTicks / refDuration, 1.0f);
        if (normalizedTime < 0.0f) normalizedTime += 1.0f;
        
        Pose pose0(skeleton);
        Pose pose1(skeleton);
        pose0.SetFromClipNormalized(samples_[0].clip.get(), normalizedTime, skeleton);
        pose1.SetFromClipNormalized(samples_[1].clip.get(), normalizedTime, skeleton);
        
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
        // Point outside all triangles - use inverse distance weighted blend
        // This provides smooth transitions even at boundaries
        
        // Calculate distances to all samples
        std::vector<std::pair<float, size_t>> distances;
        for (size_t i = 0; i < samples_.size(); ++i) {
            float dist = glm::distance(parameter, samples_[i].position);
            distances.push_back({dist, i});
        }
        
        // Sort by distance
        std::sort(distances.begin(), distances.end());
        
        // Use inverse distance weighting for closest 3 samples (or all if fewer)
        size_t blendCount = std::min(static_cast<size_t>(3), samples_.size());
        
        std::vector<float> weights(blendCount);
        float totalWeight = 0.0f;
        
        for (size_t i = 0; i < blendCount; ++i) {
            float dist = distances[i].first;
            // Inverse distance with epsilon to avoid division by zero
            weights[i] = 1.0f / (dist + 0.01f);
            totalWeight += weights[i];
        }
        
        // Normalize weights
        for (size_t i = 0; i < blendCount; ++i) {
            weights[i] /= totalWeight;
        }
        
        // Calculate normalized time using LONGEST duration for natural speed
        float maxDuration = 0.0f;
        float refTicksPerSecond = 24.0f;
        for (size_t i = 0; i < blendCount; ++i) {
            auto* clip = samples_[distances[i].second].clip.get();
            if (clip) {
                float dur = clip->GetDuration();
                if (dur > 0.0f && dur > maxDuration) {
                    maxDuration = dur;
                }
                if (i == 0) {
                    refTicksPerSecond = clip->GetTicksPerSecond();
                    if (refTicksPerSecond <= 0.0f) refTicksPerSecond = 24.0f;
                }
            }
        }
        if (maxDuration <= 0.0f) maxDuration = 24.0f;
        
        float timeInTicks = time * refTicksPerSecond;
        float normalizedTime = fmod(timeInTicks / maxDuration, 1.0f);
        if (normalizedTime < 0.0f) normalizedTime += 1.0f;
        
        // Sample and blend poses with normalized time
        Pose tempPose(skeleton);
        outPose.SetFromClipNormalized(samples_[distances[0].second].clip.get(), normalizedTime, skeleton);
        
        for (size_t i = 1; i < blendCount; ++i) {
            tempPose.SetFromClipNormalized(samples_[distances[i].second].clip.get(), normalizedTime, skeleton);
            // Accumulative blend
            float accWeight = 0.0f;
            for (size_t j = 0; j <= i; ++j) {
                accWeight += weights[j];
            }
            float blendT = weights[i] / accWeight;
            outPose.BlendWith(tempPose, blendT);
        }
        return;
    }
    
    // Blend using barycentric coordinates
    const auto& tri = triangles_[static_cast<size_t>(triangleIdx)];
    
    Pose pose0(skeleton);
    Pose pose1(skeleton);
    Pose pose2(skeleton);
    
    // Use normalized time (0.0-1.0) to keep all animations synchronized
    // Use the LONGEST duration among the 3 clips to prevent any animation from playing too fast
    auto* clip0 = samples_[static_cast<size_t>(tri[0])].clip.get();
    auto* clip1 = samples_[static_cast<size_t>(tri[1])].clip.get();
    auto* clip2 = samples_[static_cast<size_t>(tri[2])].clip.get();
    
    // Get durations (in ticks)
    float dur0 = clip0 ? clip0->GetDuration() : 24.0f;
    float dur1 = clip1 ? clip1->GetDuration() : 24.0f;
    float dur2 = clip2 ? clip2->GetDuration() : 24.0f;
    if (dur0 <= 0.0f) dur0 = 24.0f;
    if (dur1 <= 0.0f) dur1 = 24.0f;
    if (dur2 <= 0.0f) dur2 = 24.0f;
    
    // Use the LONGEST duration to ensure no animation plays faster than intended
    float refDuration = std::max({dur0, dur1, dur2});
    
    // Use first clip's ticks per second (usually consistent across all)
    float refTicksPerSecond = clip0 ? clip0->GetTicksPerSecond() : 24.0f;
    if (refTicksPerSecond <= 0.0f) refTicksPerSecond = 24.0f;
    
    // Convert time to normalized (0.0 to 1.0)
    float timeInTicks = time * refTicksPerSecond;
    float normalizedTime = fmod(timeInTicks / refDuration, 1.0f);
    if (normalizedTime < 0.0f) normalizedTime += 1.0f;
    
    pose0.SetFromClipNormalized(clip0, normalizedTime, skeleton);
    pose1.SetFromClipNormalized(clip1, normalizedTime, skeleton);
    pose2.SetFromClipNormalized(clip2, normalizedTime, skeleton);
    
    // Normalize barycentric coordinates and clamp to positive
    float w0 = std::max(0.0f, baryCoords.x);
    float w1 = std::max(0.0f, baryCoords.y);
    float w2 = std::max(0.0f, baryCoords.z);
    
    float sum = w0 + w1 + w2;
    if (sum > 0.0001f) {
        w0 /= sum;
        w1 /= sum;
        w2 /= sum;
    } else {
        w0 = w1 = w2 = 1.0f / 3.0f;
    }
    
    // Robust 3-way blend:
    // Handle cases where one or two weights dominate
    const float MIN_WEIGHT = 0.001f;
    
    // If one weight is dominant (corner case), just use that pose
    if (w0 > 0.99f) {
        outPose = pose0;
        return;
    }
    if (w1 > 0.99f) {
        outPose = pose1;
        return;
    }
    if (w2 > 0.99f) {
        outPose = pose2;
        return;
    }
    
    // Sequential blending with proper weight ratios
    outPose = pose0;
    
    // Blend pose0 with pose1
    if (w1 > MIN_WEIGHT) {
        float t1 = w1 / (w0 + w1);
        outPose.BlendWith(pose1, t1);
    }
    
    // Now outPose represents (w0 + w1) weight, blend with pose2
    if (w2 > MIN_WEIGHT) {
        outPose.BlendWith(pose2, w2);
    }
}

}  // namespace se::anim
