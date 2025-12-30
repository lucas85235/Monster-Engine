#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace se {

struct IBLResult {
    uint32_t EnvironmentCubemap = 0;    // Original HDR cubemap
    uint32_t IrradianceCubemap = 0;     // Diffuse convolution
    uint32_t PrefilteredCubemap = 0;    // Specular prefiltered (mipmapped)
    uint32_t DfgLut = 0;                // Split-sum LUT
    int CubemapSize = 512;
    int PrefilteredMipLevels = 5;
    int LutSize = 128;
    bool Valid = false;
};

class IBLProcessor {
public:
    static IBLResult ProcessHDR(const std::filesystem::path& hdrPath, int cubemapSize = 512);
    static uint32_t GenerateDfgLut(int size = 128);
    static void Cleanup(IBLResult& result);
    
private:
    static uint32_t LoadHDRTexture(const std::filesystem::path& path);
    static uint32_t EquirectToCubemap(uint32_t equirectTex, int size);
    static uint32_t GenerateIrradianceCubemap(uint32_t envCubemap, int size);
    static uint32_t GeneratePrefilteredCubemap(uint32_t envCubemap, int baseSize, int& outMipLevels);
    
    static uint32_t CreateCubemapShader(const std::string& vertSrc, const std::string& fragSrc);
    static void RenderToCubemap(uint32_t shader, uint32_t cubemap, int size, int mipLevel = 0);
    
    static uint32_t cubemapVAO_;
    static uint32_t cubemapVBO_;
    static bool initialized_;
    
    static void InitCubeGeometry();
};

}  // namespace se
