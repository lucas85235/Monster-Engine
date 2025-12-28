#pragma once

#include <glm.hpp>
#include <array>

namespace se {
namespace gi {

struct SHL2Probe {
    std::array<glm::vec3, 9> coefficients{};
    
    void Clear() {
        for (auto& c : coefficients) {
            c = glm::vec3(0.0f);
        }
    }
    
    void AddSample(const glm::vec3& direction, const glm::vec3& radiance, float weight = 1.0f);
    
    glm::vec3 Evaluate(const glm::vec3& normal) const;
    
    void Normalize(int sampleCount);
    
    static constexpr float kSHBasis0 = 0.282095f;       // Y_0^0
    static constexpr float kSHBasis1 = 0.488603f;       // Y_1^{-1}, Y_1^0, Y_1^1
    static constexpr float kSHBasis2a = 1.092548f;      // Y_2^{-2}, Y_2^2
    static constexpr float kSHBasis2b = 0.315392f;      // Y_2^0
    static constexpr float kSHBasis2c = 0.546274f;      // Y_2^{-1}, Y_2^1
};

inline void SHL2Probe::AddSample(const glm::vec3& direction, const glm::vec3& radiance, float weight) {
    const float x = direction.x;
    const float y = direction.y;
    const float z = direction.z;
    
    // L0
    coefficients[0] += radiance * kSHBasis0 * weight;
    
    // L1
    coefficients[1] += radiance * kSHBasis1 * y * weight;
    coefficients[2] += radiance * kSHBasis1 * z * weight;
    coefficients[3] += radiance * kSHBasis1 * x * weight;
    
    // L2
    coefficients[4] += radiance * kSHBasis2a * x * y * weight;
    coefficients[5] += radiance * kSHBasis2c * y * z * weight;
    coefficients[6] += radiance * kSHBasis2b * (3.0f * z * z - 1.0f) * weight;
    coefficients[7] += radiance * kSHBasis2c * x * z * weight;
    coefficients[8] += radiance * kSHBasis2a * (x * x - y * y) * weight;
}

inline glm::vec3 SHL2Probe::Evaluate(const glm::vec3& normal) const {
    const float x = normal.x;
    const float y = normal.y;
    const float z = normal.z;
    
    glm::vec3 result{0.0f};
    
    // Clamped cosine lobe convolution for diffuse irradiance
    // These are the cosine lobe SH coefficients (A_l factors)
    constexpr float kA0 = 3.141593f;            // π
    constexpr float kA1 = 2.094395f;            // 2π/3
    constexpr float kA2 = 0.785398f;            // π/4
    
    // L0
    result += coefficients[0] * kSHBasis0 * kA0;
    
    // L1
    result += coefficients[1] * kSHBasis1 * y * kA1;
    result += coefficients[2] * kSHBasis1 * z * kA1;
    result += coefficients[3] * kSHBasis1 * x * kA1;
    
    // L2
    result += coefficients[4] * kSHBasis2a * x * y * kA2;
    result += coefficients[5] * kSHBasis2c * y * z * kA2;
    result += coefficients[6] * kSHBasis2b * (3.0f * z * z - 1.0f) * kA2;
    result += coefficients[7] * kSHBasis2c * x * z * kA2;
    result += coefficients[8] * kSHBasis2a * (x * x - y * y) * kA2;
    
    return glm::max(result, glm::vec3(0.0f));
}

inline void SHL2Probe::Normalize(int sampleCount) {
    if (sampleCount > 0) {
        const float invCount = 4.0f * 3.14159265f / static_cast<float>(sampleCount);
        for (auto& c : coefficients) {
            c *= invCount;
        }
    }
}

}  // namespace gi
}  // namespace se
