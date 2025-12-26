#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace se {

// Renamed to avoid collision with Buffer.h's ShaderDataType
enum class ReflectionDataType { None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool, Sampler2D, SamplerCube };

struct ShaderResourceDeclaration {
    std::string        name;
    uint32_t           set     = 0;
    uint32_t           binding = 0;
    uint32_t           size    = 0;
    uint32_t           offset  = 0;
    ReflectionDataType type    = ReflectionDataType::None;  // Uses distinct type
    uint32_t           count   = 1;                         // Array size
};

struct ShaderUniformBufferDeclaration {
    std::string                            name;
    uint32_t                               set     = 0;
    uint32_t                               binding = 0;
    uint32_t                               size    = 0;
    std::vector<ShaderResourceDeclaration> members;
};

struct ShaderReflectionData {
    std::vector<ShaderUniformBufferDeclaration> uniformBuffers;
    std::vector<ShaderResourceDeclaration>      resources;  // Textures, samplers
    std::vector<ShaderResourceDeclaration>      pushConstants;
};

}  // namespace se
