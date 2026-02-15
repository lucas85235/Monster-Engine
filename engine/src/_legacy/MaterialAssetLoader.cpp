#include "engine/assets/MaterialAssetLoader.h"

#include <fstream>

#include <glad/glad.h>

#include "engine/assets/MaterialAssetSerializer.h"
#include "engine/renderer/MaterialInstance.h"
#include "engine/renderer/Texture.h"
#include "engine/resources/TextureManager.h"
#include "engine/Log.h"

namespace se {

std::shared_ptr<MaterialInstance> MaterialAssetLoader::Load(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        SE_LOG_ERROR("MaterialAssetLoader: File not found: {}", path.string());
        return nullptr;
    }
    
    SE_LOG_INFO("MaterialAssetLoader: Loading material from '{}'", path.string());
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        SE_LOG_ERROR("MaterialAssetLoader: Failed to open file: {}", path.string());
        return nullptr;
    }
    
    MaterialAsset asset;
    MaterialAssetSerializer serializer;
    
    if (!serializer.Deserialize(asset, file)) {
        SE_LOG_ERROR("MaterialAssetLoader: Failed to deserialize material");
        return nullptr;
    }
    
    asset.definition.sourcePath = path.string();
    
    return LoadFromAsset(asset);
}

std::shared_ptr<MaterialInstance> MaterialAssetLoader::LoadFromAsset(const MaterialAsset& asset) {
    SE_LOG_INFO("MaterialAssetLoader: Creating MaterialInstance from asset '{}'", asset.name);
    
    auto instance = MaterialInstance::Create(asset.definition);
    
    if (asset.albedoTexture && asset.albedoTexture->IsValid()) {
        auto tex = CreateTextureFromEmbedded(*asset.albedoTexture);
        if (tex) {
            instance->SetTexture(TextureSlot::Albedo, tex);
            SE_LOG_INFO("MaterialAssetLoader: Loaded embedded albedo texture ({}x{})",
                        asset.albedoTexture->header.width, asset.albedoTexture->header.height);
        }
    } else if (!asset.albedoSourcePath.empty()) {
        auto tex = TextureManager::Load(asset.albedoSourcePath);
        if (tex) {
            instance->SetTexture(TextureSlot::Albedo, tex);
        }
    }
    
    if (asset.normalTexture && asset.normalTexture->IsValid()) {
        auto tex = CreateTextureFromEmbedded(*asset.normalTexture);
        if (tex) {
            instance->SetTexture(TextureSlot::Normal, tex);
            SE_LOG_INFO("MaterialAssetLoader: Loaded embedded normal texture ({}x{})",
                        asset.normalTexture->header.width, asset.normalTexture->header.height);
        }
    } else if (!asset.normalSourcePath.empty()) {
        auto tex = TextureManager::Load(asset.normalSourcePath);
        if (tex) {
            instance->SetTexture(TextureSlot::Normal, tex);
        }
    }
    
    if (asset.metallicTexture && asset.metallicTexture->IsValid()) {
        auto tex = CreateTextureFromEmbedded(*asset.metallicTexture);
        if (tex) {
            instance->SetTexture(TextureSlot::Metallic, tex);
        }
    } else if (!asset.metallicSourcePath.empty()) {
        auto tex = TextureManager::Load(asset.metallicSourcePath);
        if (tex) {
            instance->SetTexture(TextureSlot::Metallic, tex);
        }
    }
    
    if (asset.roughnessTexture && asset.roughnessTexture->IsValid()) {
        auto tex = CreateTextureFromEmbedded(*asset.roughnessTexture);
        if (tex) {
            instance->SetTexture(TextureSlot::Roughness, tex);
        }
    } else if (!asset.roughnessSourcePath.empty()) {
        auto tex = TextureManager::Load(asset.roughnessSourcePath);
        if (tex) {
            instance->SetTexture(TextureSlot::Roughness, tex);
        }
    }
    
    if (asset.aoTexture && asset.aoTexture->IsValid()) {
        auto tex = CreateTextureFromEmbedded(*asset.aoTexture);
        if (tex) {
            instance->SetTexture(TextureSlot::AO, tex);
        }
    } else if (!asset.aoSourcePath.empty()) {
        auto tex = TextureManager::Load(asset.aoSourcePath);
        if (tex) {
            instance->SetTexture(TextureSlot::AO, tex);
        }
    }
    
    if (asset.emissiveTexture && asset.emissiveTexture->IsValid()) {
        auto tex = CreateTextureFromEmbedded(*asset.emissiveTexture);
        if (tex) {
            instance->SetTexture(TextureSlot::Emissive, tex);
        }
    } else if (!asset.emissiveSourcePath.empty()) {
        auto tex = TextureManager::Load(asset.emissiveSourcePath);
        if (tex) {
            instance->SetTexture(TextureSlot::Emissive, tex);
        }
    }
    
    instance->MarkDirty();
    
    SE_LOG_INFO("MaterialAssetLoader: MaterialInstance '{}' created successfully", asset.name);
    return instance;
}

std::shared_ptr<Texture> MaterialAssetLoader::CreateTextureFromEmbedded(const EmbeddedTextureData& data) {
    if (!data.IsValid()) {
        SE_LOG_WARN("MaterialAssetLoader: Invalid embedded texture data");
        return nullptr;
    }
    
    return Texture::CreateFromMemory(
        data.pixels.data(),
        data.header.width,
        data.header.height,
        data.header.channels,
        "embedded_material_texture"
    );
}

bool MaterialAssetLoader::SaveAsset(const MaterialAsset& asset, const std::filesystem::path& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        SE_LOG_ERROR("MaterialAssetLoader: Failed to create file: {}", path.string());
        return false;
    }
    
    MaterialAssetSerializer serializer;
    serializer.EmbedTextures(true);
    
    if (!serializer.Serialize(asset, file)) {
        SE_LOG_ERROR("MaterialAssetLoader: Failed to serialize material");
        return false;
    }
    
    SE_LOG_INFO("MaterialAssetLoader: Saved material to '{}'", path.string());
    return true;
}

void MaterialAssetLoader::UploadTextureToGPU(uint32_t textureId, const EmbeddedTextureData& data) {
    if (textureId == 0 || !data.IsValid()) return;
    
    glBindTexture(GL_TEXTURE_2D, textureId);
    
    GLenum internalFormat = GL_RGBA8;
    GLenum dataFormat = GL_RGBA;
    
    switch (data.header.channels) {
        case 4: internalFormat = GL_RGBA8; dataFormat = GL_RGBA; break;
        case 3: internalFormat = GL_RGB8;  dataFormat = GL_RGB;  break;
        case 2: internalFormat = GL_RG8;   dataFormat = GL_RG;   break;
        case 1: internalFormat = GL_R8;    dataFormat = GL_RED;  break;
    }
    
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        internalFormat,
        data.header.width,
        data.header.height,
        0,
        dataFormat,
        GL_UNSIGNED_BYTE,
        data.pixels.data()
    );
    
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

}  // namespace se
