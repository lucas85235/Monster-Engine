#include "engine/renderer/DFGLutGenerator.h"
#include "engine/renderer/Texture.h"
#include "engine/Log.h"

#include <glad/glad.h>
#include <cmath>
#include <vector>
#include <fstream>

namespace se {

namespace {
    constexpr float PI = 3.14159265359f;
    constexpr uint32_t SAMPLE_COUNT = 1024;
}

float DFGLutGenerator::RadicalInverse_VdC(uint32_t bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f;
}

void DFGLutGenerator::Hammersley(uint32_t i, uint32_t N, float& xi1, float& xi2) {
    xi1 = float(i) / float(N);
    xi2 = RadicalInverse_VdC(i);
}

void DFGLutGenerator::ImportanceSampleGGX(float xi1, float xi2, float roughness, float& NoH, float& VoH) {
    float a = roughness * roughness;
    float phi = 2.0f * PI * xi1;
    float cosTheta = std::sqrt((1.0f - xi2) / (1.0f + (a * a - 1.0f) * xi2));
    float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);
    
    float Hx = sinTheta * std::cos(phi);
    float Hy = sinTheta * std::sin(phi);
    float Hz = cosTheta;
    
    NoH = Hz;
    VoH = Hz;
}

float DFGLutGenerator::GeometrySchlickGGX(float NdotV, float roughness) {
    float a = roughness;
    float k = (a * a) / 2.0f;
    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;
    return nom / denom;
}

float DFGLutGenerator::GeometrySmith(float NoV, float NoL, float roughness) {
    float ggx2 = GeometrySchlickGGX(NoV, roughness);
    float ggx1 = GeometrySchlickGGX(NoL, roughness);
    return ggx1 * ggx2;
}

void DFGLutGenerator::IntegrateBRDF(float NoV, float roughness, float& scale, float& bias) {
    // View vector in tangent space (assuming N = (0,0,1))
    float Vx = std::sqrt(1.0f - NoV * NoV);
    float Vy = 0.0f;
    float Vz = NoV;
    
    float A = 0.0f;
    float B = 0.0f;
    
    for (uint32_t i = 0; i < SAMPLE_COUNT; ++i) {
        float xi1, xi2;
        Hammersley(i, SAMPLE_COUNT, xi1, xi2);
        
        float a = roughness * roughness;
        float phi = 2.0f * PI * xi1;
        float cosTheta = std::sqrt((1.0f - xi2) / (1.0f + (a * a - 1.0f) * xi2));
        float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);
        
        float Hx = sinTheta * std::cos(phi);
        float Hy = sinTheta * std::sin(phi);
        float Hz = cosTheta;
        
        float VdotH = Vx * Hx + Vy * Hy + Vz * Hz;
        
        float Lx = 2.0f * VdotH * Hx - Vx;
        float Ly = 2.0f * VdotH * Hy - Vy;
        float Lz = 2.0f * VdotH * Hz - Vz;
        
        float NoL = std::max(Lz, 0.0f);
        float NoH = std::max(Hz, 0.0f);
        VdotH = std::max(VdotH, 0.0f);
        
        if (NoL > 0.0f) {
            float G = GeometrySmith(NoV, NoL, roughness);
            float G_Vis = (G * VdotH) / (NoH * NoV);
            float Fc = std::pow(1.0f - VdotH, 5.0f);
            
            A += (1.0f - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    
    scale = A / float(SAMPLE_COUNT);
    bias = B / float(SAMPLE_COUNT);
}

std::shared_ptr<Texture> DFGLutGenerator::Generate() {
    SE_LOG_INFO("DFGLutGenerator: Generating {}x{} DFG LUT...", LUT_SIZE, LUT_SIZE);
    
    std::vector<float> data(LUT_SIZE * LUT_SIZE * 2);
    
    for (int y = 0; y < LUT_SIZE; ++y) {
        float roughness = (float(y) + 0.5f) / float(LUT_SIZE);
        roughness = std::max(roughness, 0.01f);
        
        for (int x = 0; x < LUT_SIZE; ++x) {
            float NoV = (float(x) + 0.5f) / float(LUT_SIZE);
            NoV = std::max(NoV, 0.001f);
            
            float scale, bias;
            IntegrateBRDF(NoV, roughness, scale, bias);
            
            int idx = (y * LUT_SIZE + x) * 2;
            data[idx + 0] = scale;
            data[idx + 1] = bias;
        }
    }
    
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, LUT_SIZE, LUT_SIZE, 0, GL_RG, GL_FLOAT, data.data());
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    auto texture = std::make_shared<Texture>();
    texture->SetFromGLHandle(textureId, LUT_SIZE, LUT_SIZE, 2, "[DFG_LUT]");
    
    SE_LOG_INFO("DFGLutGenerator: LUT generated successfully");
    return texture;
}

std::shared_ptr<Texture> DFGLutGenerator::LoadOrGenerate(const std::string& cachePath) {
    if (!cachePath.empty()) {
        std::ifstream file(cachePath, std::ios::binary);
        if (file.good()) {
            SE_LOG_INFO("DFGLutGenerator: Loading cached LUT from '{}'", cachePath);
            
            std::vector<float> data(LUT_SIZE * LUT_SIZE * 2);
            file.read(reinterpret_cast<char*>(data.data()), data.size() * sizeof(float));
            
            if (file.good()) {
                GLuint textureId;
                glGenTextures(1, &textureId);
                glBindTexture(GL_TEXTURE_2D, textureId);
                
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, LUT_SIZE, LUT_SIZE, 0, GL_RG, GL_FLOAT, data.data());
                
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                
                glBindTexture(GL_TEXTURE_2D, 0);
                
                auto texture = std::make_shared<Texture>();
                texture->SetFromGLHandle(textureId, LUT_SIZE, LUT_SIZE, 2, "[DFG_LUT_Cached]");
                return texture;
            }
        }
    }
    
    auto lut = Generate();
    
    if (!cachePath.empty() && lut) {
        SaveToFile(lut, cachePath);
    }
    
    return lut;
}

bool DFGLutGenerator::SaveToFile(const std::shared_ptr<Texture>& lut, const std::string& path) {
    if (!lut || !lut->IsValid()) return false;
    
    std::vector<float> data(LUT_SIZE * LUT_SIZE * 2);
    
    glBindTexture(GL_TEXTURE_2D, lut->GetId());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, data.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    
    std::ofstream file(path, std::ios::binary);
    if (!file.good()) {
        SE_LOG_ERROR("DFGLutGenerator: Failed to save LUT to '{}'", path);
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(float));
    SE_LOG_INFO("DFGLutGenerator: Saved LUT to '{}'", path);
    return true;
}

} // namespace se
