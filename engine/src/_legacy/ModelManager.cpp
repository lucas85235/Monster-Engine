#include "engine/resources/ModelManager.h"

#include <filesystem>

#include <glad/glad.h>
#include <glm.hpp>
#include "engine/Log.h"
#include "engine/renderer/Buffer.h"
#include "engine/renderer/Material.h"
#include "engine/renderer/MaterialInstance.h"
#include "engine/renderer/Texture.h"
#include "engine/renderer/TextureMaterial.h"
#include "engine/renderer/VertexArray.h"
#include "engine/resources/AssimpModelLoader.h"
#include "engine/resources/MaterialLibrary.h"
#include "engine/resources/Model.h"
#include "engine/resources/ModelData.h"
#include "engine/resources/SubMesh.h"
#include "engine/resources/TextureManager.h"

namespace se {

std::unique_ptr<IModelLoader> ModelManager::loader_;
std::unordered_map<std::string, std::shared_ptr<Model>> ModelManager::cache_;
bool ModelManager::initialized_ = false;

void ModelManager::Init() {
    if (initialized_) {
        SE_LOG_WARN("ModelManager already initialized");
        return;
    }

    SE_LOG_INFO("Initializing ModelManager");
    loader_ = std::make_unique<AssimpModelLoader>();
    cache_.clear();
    initialized_ = true;
}

void ModelManager::Shutdown() {
    if (!initialized_) return;

    SE_LOG_INFO("Shutting down ModelManager");
    ClearCache();
    loader_.reset();
    initialized_ = false;
}

std::shared_ptr<Model> ModelManager::Load(const std::string& path) {
    if (!initialized_) {
        SE_LOG_ERROR("ModelManager not initialized!");
        return nullptr;
    }

    std::filesystem::path filePath(path);
    std::string name = filePath.stem().string();

    auto it = cache_.find(name);
    if (it != cache_.end()) {
        SE_LOG_INFO("ModelManager: Model '{}' found in cache", name);
        return it->second;
    }

    SE_LOG_INFO("ModelManager: Loading model from '{}'", path);

    auto modelData = loader_->Load(path);
    if (!modelData || !modelData->IsValid()) {
        SE_LOG_ERROR("ModelManager: Failed to load model from '{}'", path);
        return nullptr;
    }

    auto model = CreateModelFromData(*modelData);
    if (!model) {
        SE_LOG_ERROR("ModelManager: Failed to create GPU resources for model '{}'", name);
        return nullptr;
    }

    cache_[name] = model;
    SE_LOG_INFO("ModelManager: Model '{}' loaded and cached", name);

    return model;
}

std::shared_ptr<Model> ModelManager::Get(const std::string& name) {
    auto it = cache_.find(name);
    if (it != cache_.end()) {
        return it->second;
    }
    return nullptr;
}

bool ModelManager::Has(const std::string& name) {
    return cache_.find(name) != cache_.end();
}

void ModelManager::Unload(const std::string& name) {
    auto it = cache_.find(name);
    if (it != cache_.end()) {
        SE_LOG_INFO("ModelManager: Unloading model '{}'", name);
        cache_.erase(it);
    }
}

void ModelManager::ClearCache() {
    SE_LOG_INFO("ModelManager: Clearing cache ({} models)", cache_.size());
    cache_.clear();
}

// PBR Vertex layout matching model.vert shader:
// layout(location = 0) in vec3 a_Position;
// layout(location = 1) in vec3 a_Normal;
// layout(location = 2) in vec2 a_TexCoord;
// layout(location = 3) in vec3 a_Tangent;
// layout(location = 4) in vec3 a_Bitangent;
struct PBRVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

std::shared_ptr<TextureMaterial> ModelManager::CreateTextureMaterial(const MaterialData& matData) {
    auto texMat = std::make_shared<TextureMaterial>();
    
    texMat->BaseColor = matData.DiffuseColor;
    texMat->MetallicFactor = matData.Metallic;
    texMat->RoughnessFactor = matData.Roughness;
    texMat->Shininess = matData.Shininess;
    texMat->EmissiveColor = matData.EmissiveColor;
    
    if (!matData.DiffuseTexturePath.empty()) {
        texMat->Albedo = TextureManager::Load(matData.DiffuseTexturePath);
        if (texMat->Albedo) {
            SE_LOG_INFO("ModelManager: Loaded albedo texture '{}'", matData.DiffuseTexturePath);
        }
    }
    
    if (!matData.NormalTexturePath.empty()) {
        texMat->Normal = TextureManager::Load(matData.NormalTexturePath);
        if (texMat->Normal) {
            SE_LOG_INFO("ModelManager: Loaded normal texture '{}'", matData.NormalTexturePath);
        }
    }
    
    if (!matData.SpecularTexturePath.empty()) {
        texMat->Specular = TextureManager::Load(matData.SpecularTexturePath);
        if (texMat->Specular) {
            SE_LOG_INFO("ModelManager: Loaded specular texture '{}'", matData.SpecularTexturePath);
        }
    }
    
    if (!matData.AOTexturePath.empty()) {
        texMat->AO = TextureManager::Load(matData.AOTexturePath);
        if (texMat->AO) {
            SE_LOG_INFO("ModelManager: Loaded AO texture '{}'", matData.AOTexturePath);
        }
    }
    
    if (!matData.EmissiveTexturePath.empty()) {
        texMat->Emissive = TextureManager::Load(matData.EmissiveTexturePath);
        if (texMat->Emissive) {
            SE_LOG_INFO("ModelManager: Loaded emissive texture '{}'", matData.EmissiveTexturePath);
        }
    }
    
    if (!matData.RoughnessTexturePath.empty()) {
        texMat->Roughness = TextureManager::Load(matData.RoughnessTexturePath);
        if (texMat->Roughness) {
            SE_LOG_INFO("ModelManager: Loaded roughness texture '{}'", matData.RoughnessTexturePath);
        }
    }
    
    if (!matData.MetallicTexturePath.empty()) {
        texMat->Metallic = TextureManager::Load(matData.MetallicTexturePath);
        if (texMat->Metallic) {
            SE_LOG_INFO("ModelManager: Loaded metallic texture '{}'", matData.MetallicTexturePath);
        }
    }
    
    return texMat;
}

std::shared_ptr<Model> ModelManager::CreateModelFromData(const ModelData& data) {
    auto model = std::make_shared<Model>(data.Name);
    model->SetBoundingBox(data.Bounds);

    for (const auto& submeshData : data.SubMeshes) {
        if (submeshData.Vertices.empty()) {
            SE_LOG_WARN("ModelManager: Skipping empty submesh '{}'", submeshData.Name);
            continue;
        }

        // Convert ModelVertex to PBRVertex (matching model.vert layout)
        std::vector<PBRVertex> vertices;
        vertices.reserve(submeshData.Vertices.size());
        
        for (const auto& v : submeshData.Vertices) {
            PBRVertex pv;
            pv.Position = v.Position;
            pv.Normal = v.Normal;
            pv.TexCoord = v.TexCoord;
            pv.Tangent = v.Tangent;
            pv.Bitangent = v.Bitangent;
            vertices.push_back(pv);
        }

        // Create VertexArray with PBR layout
        auto vertexArray = std::make_shared<VertexArray>();
        
        auto vertexBuffer = std::make_shared<VertexBuffer>(
            vertices.data(),
            static_cast<uint32_t>(vertices.size() * sizeof(PBRVertex))
        );
        vertexBuffer->SetLayout(BufferLayout({
            {ShaderDataType::Float3, "a_Position"},
            {ShaderDataType::Float3, "a_Normal"},
            {ShaderDataType::Float2, "a_TexCoord"},
            {ShaderDataType::Float3, "a_Tangent"},
            {ShaderDataType::Float3, "a_Bitangent"}
        }));
        vertexArray->AddVertexBuffer(vertexBuffer);
        
        auto indexBuffer = std::make_shared<IndexBuffer>(
            submeshData.Indices.data(),
            static_cast<uint32_t>(submeshData.Indices.size())
        );
        vertexArray->SetIndexBuffer(indexBuffer);
        
        vertexArray->Unbind();

        // Create TextureMaterial from material data
        std::shared_ptr<TextureMaterial> textureMaterial = nullptr;
        if (submeshData.MaterialIndex >= 0 && 
            submeshData.MaterialIndex < static_cast<int>(data.Materials.size())) {
            textureMaterial = CreateTextureMaterial(data.Materials[submeshData.MaterialIndex]);
        }

        // Material will be set by the user/renderer
        std::shared_ptr<Material> material = nullptr;

        SubMesh submesh(vertexArray, material, submeshData.Name);
        if (textureMaterial) {
            submesh.SetTextureMaterial(textureMaterial);
            
            // Also create MaterialInstance for new system
            std::string matName = data.Name + "_" + submeshData.Name + "_mat";
            MaterialInstance* matInstance = MaterialLibrary::Get().CreateFromTextureMaterial(
                matName,
                textureMaterial->Albedo,
                textureMaterial->Normal,
                textureMaterial->Metallic,
                textureMaterial->Roughness,
                textureMaterial->AO,
                textureMaterial->Emissive
            );
            if (matInstance) {
                matInstance->SetBaseColor(textureMaterial->BaseColor);
                matInstance->SetMetallic(textureMaterial->MetallicFactor);
                matInstance->SetRoughness(textureMaterial->RoughnessFactor);
                submesh.SetMaterialInstance(matInstance);
                SE_LOG_INFO("ModelManager: Created MaterialInstance '{}'", matName);
            }
        }
        model->AddSubMesh(std::move(submesh));

        SE_LOG_INFO("ModelManager: Created submesh '{}' with {} vertices, {} indices",
                    submeshData.Name, vertices.size(), submeshData.Indices.size());
    }

    return model;
}

}  // namespace se
