#pragma once
/**
 * MaterialAssetLoader.h - Material asset loading with GPU texture creation.
 *
 * Loads MaterialAsset from binary files and creates MaterialInstance
 * with GPU-resident textures from embedded data.
 */

#include <filesystem>
#include <memory>

#include "engine/assets/MaterialAsset.h"

namespace se {

class MaterialInstance;
class Texture;

class MaterialAssetLoader {
public:
    static std::shared_ptr<MaterialInstance> Load(const std::filesystem::path& path);
    
    static std::shared_ptr<MaterialInstance> LoadFromAsset(const MaterialAsset& asset);
    
    static std::shared_ptr<Texture> CreateTextureFromEmbedded(const EmbeddedTextureData& data);
    
    static bool SaveAsset(const MaterialAsset& asset, const std::filesystem::path& path);
    
private:
    static void UploadTextureToGPU(
        uint32_t textureId,
        const EmbeddedTextureData& data
    );
};

}  // namespace se
