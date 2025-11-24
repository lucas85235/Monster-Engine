#pragma once
#include "engine/utils/FilesHandler.h"
#include "se_pch.h"

namespace Renderer {
class Shader {
    uint32_t CreateComputeShader(const std::filesystem::path& path);
    uint32_t ReloadComputeShader(uint32_t shaderHandle, const std::filesystem::path& path);

    uint32_t CreateGraphicsShader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
    uint32_t ReloadGraphicsShader(uint32_t shaderHandle, const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
};
}  // namespace Renderer
