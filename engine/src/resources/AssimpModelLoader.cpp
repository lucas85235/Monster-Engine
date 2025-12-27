#include "engine/resources/AssimpModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>
#include <algorithm>

#include "engine/Log.h"
#include "engine/resources/ModelData.h"

namespace se {

std::unique_ptr<ModelData> AssimpModelLoader::Load(const std::string& path) {
    SE_LOG_INFO("AssimpModelLoader: Loading model from '{}'", path);

    Assimp::Importer importer;
    
    unsigned int flags = aiProcess_Triangulate |       // Convert quads/n-gons to triangles
                         aiProcess_GenNormals |        // Generate flat normals if missing
                         aiProcess_FlipUVs |           // Flip Y-axis of texture coords for OpenGL
                         aiProcess_CalcTangentSpace |  // Calculate tangent and bitangent
                         aiProcess_SortByPType;        // Split meshes by primitive type

    const aiScene* scene = importer.ReadFile(path, flags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        SE_LOG_ERROR("AssimpModelLoader: Failed to load model '{}': {}", path, importer.GetErrorString());
        return nullptr;
    }

    auto modelData = std::make_unique<ModelData>();
    modelData->SourcePath = path;
    
    std::filesystem::path filePath(path);
    modelData->Name = filePath.stem().string();
    directory_ = filePath.parent_path().string();

    SE_LOG_INFO("AssimpModelLoader: Scene has {} meshes, {} materials", 
                scene->mNumMeshes, scene->mNumMaterials);

    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        modelData->Materials.push_back(ProcessMaterial(scene->mMaterials[i], directory_));
    }

    ProcessNode(scene->mRootNode, scene, *modelData);

    modelData->Bounds.Min = glm::vec3(std::numeric_limits<float>::max());
    modelData->Bounds.Max = glm::vec3(std::numeric_limits<float>::lowest());
    
    for (const auto& submesh : modelData->SubMeshes) {
        for (const auto& vertex : submesh.Vertices) {
            modelData->Bounds.Min = glm::min(modelData->Bounds.Min, vertex.Position);
            modelData->Bounds.Max = glm::max(modelData->Bounds.Max, vertex.Position);
        }
    }

    SE_LOG_INFO("AssimpModelLoader: Loaded model '{}' with {} submeshes, {} materials",
                modelData->Name, modelData->SubMeshes.size(), modelData->Materials.size());

    return modelData;
}

bool AssimpModelLoader::SupportsFormat(const std::string& extension) const {
    static const std::vector<std::string> supportedFormats = {
        ".fbx", ".obj", ".gltf", ".glb", ".dae", ".3ds", ".blend",
        ".stl", ".ply", ".x", ".ms3d", ".cob", ".smd", ".vta",
        ".mdl", ".md2", ".md3", ".pk3", ".mdc", ".x3d", ".lwo",
        ".lws", ".ac", ".ase", ".ifc", ".bvh", ".csm", ".dxf",
        ".hmp", ".irr", ".irrmesh", ".nff", ".ndo", ".off", ".q3o",
        ".q3s", ".raw", ".ter", ".xgl", ".zgl"
    };

    std::string ext = extension;
    if (!ext.empty() && ext[0] != '.') {
        ext = "." + ext;
    }
    
    for (auto& c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    for (const auto& format : supportedFormats) {
        if (format == ext) {
            return true;
        }
    }
    return false;
}

void AssimpModelLoader::ProcessNode(const aiNode* node, const aiScene* scene, ModelData& modelData) {
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        modelData.SubMeshes.push_back(ProcessMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        ProcessNode(node->mChildren[i], scene, modelData);
    }
}

SubMeshData AssimpModelLoader::ProcessMesh(const aiMesh* mesh, const aiScene* scene) {
    SubMeshData submesh;
    submesh.Name = mesh->mName.C_Str();
    submesh.MaterialIndex = static_cast<int>(mesh->mMaterialIndex);

    SE_LOG_INFO("AssimpModelLoader: Processing mesh '{}' with {} vertices, {} faces",
                submesh.Name, mesh->mNumVertices, mesh->mNumFaces);

    submesh.Vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        ModelVertex vertex;
        
        vertex.Position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );

        if (mesh->HasNormals()) {
            vertex.Normal = glm::vec3(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            );
        }

        if (mesh->mTextureCoords[0]) {
            vertex.TexCoord = glm::vec2(
                mesh->mTextureCoords[0][i].x,
                1.0f - mesh->mTextureCoords[0][i].y  // Flip Y for OpenGL compatibility
            );
        }

        if (mesh->HasTangentsAndBitangents()) {
            vertex.Tangent = glm::vec3(
                mesh->mTangents[i].x,
                mesh->mTangents[i].y,
                mesh->mTangents[i].z
            );
            vertex.Bitangent = glm::vec3(
                mesh->mBitangents[i].x,
                mesh->mBitangents[i].y,
                mesh->mBitangents[i].z
            );
        }

        submesh.Vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            submesh.Indices.push_back(face.mIndices[j]);
        }
    }

    SE_LOG_INFO("AssimpModelLoader: Mesh '{}' processed: {} vertices, {} indices",
                submesh.Name, submesh.Vertices.size(), submesh.Indices.size());

    return submesh;
}

MaterialData AssimpModelLoader::ProcessMaterial(const aiMaterial* material, const std::string& directory) {
    MaterialData matData;

    aiString name;
    if (material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
        matData.Name = name.C_Str();
    }

    aiColor4D color;
    if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
        matData.DiffuseColor = glm::vec4(color.r, color.g, color.b, color.a);
    }
    if (material->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
        matData.SpecularColor = glm::vec4(color.r, color.g, color.b, color.a);
    }
    if (material->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
        matData.AmbientColor = glm::vec4(color.r, color.g, color.b, color.a);
    }

    float shininess;
    if (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
        matData.Shininess = shininess;
    }

    // Helper lambda to extract texture path with existence check
    auto extractTexture = [&](aiTextureType type) -> std::string {
        aiString texPath;
        if (material->GetTexture(type, 0, &texPath) == AI_SUCCESS) {
            std::filesystem::path fullPath = std::filesystem::path(directory) / texPath.C_Str();
            if (std::filesystem::exists(fullPath)) {
                return fullPath.string();
            } else {
                SE_LOG_WARN("AssimpModelLoader: Texture file not found '{}'", fullPath.string());
            }
        }
        return "";
    };

    matData.DiffuseTexturePath = extractTexture(aiTextureType_DIFFUSE);
    matData.NormalTexturePath = extractTexture(aiTextureType_NORMALS);
    
    // Try HEIGHT as fallback for normal map (some exporters use this)
    if (matData.NormalTexturePath.empty()) {
        matData.NormalTexturePath = extractTexture(aiTextureType_HEIGHT);
    }
    
    matData.SpecularTexturePath = extractTexture(aiTextureType_SPECULAR);
    matData.AOTexturePath = extractTexture(aiTextureType_AMBIENT_OCCLUSION);
    
    // Fallback: try LIGHTMAP for AO (some formats use this)
    if (matData.AOTexturePath.empty()) {
        matData.AOTexturePath = extractTexture(aiTextureType_LIGHTMAP);
    }
    
    matData.EmissiveTexturePath = extractTexture(aiTextureType_EMISSIVE);
    matData.RoughnessTexturePath = extractTexture(aiTextureType_DIFFUSE_ROUGHNESS);
    matData.MetallicTexturePath = extractTexture(aiTextureType_METALNESS);

    // Auto-discovery: If no textures found, try to find them by naming convention
    // Common patterns: *_Albedo.*, *_Diffuse.*, *_Normal.*, *_Roughness.*, *_AO.*, *_Occlusion.*, etc.
    if (matData.DiffuseTexturePath.empty() || matData.NormalTexturePath.empty()) {
        SE_LOG_INFO("AssimpModelLoader: Attempting auto-discovery of textures in '{}'", directory);
        
        std::vector<std::string> extensions = {".png", ".jpg", ".jpeg", ".tga", ".bmp"};
        
        auto findTexture = [&](const std::vector<std::string>& patterns) -> std::string {
            for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                if (!entry.is_regular_file()) continue;
                
                std::string filename = entry.path().filename().string();
                std::string lowerFilename = filename;
                std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);
                
                for (const auto& pattern : patterns) {
                    if (lowerFilename.find(pattern) != std::string::npos) {
                        SE_LOG_INFO("AssimpModelLoader: Auto-discovered texture '{}' for pattern '{}'", 
                                    filename, pattern);
                        return entry.path().string();
                    }
                }
            }
            return "";
        };
        
        // Albedo/Diffuse
        if (matData.DiffuseTexturePath.empty()) {
            matData.DiffuseTexturePath = findTexture({"_albedo", "_diffuse", "_basecolor", "_color", "_d."});
        }
        
        // Normal
        if (matData.NormalTexturePath.empty()) {
            matData.NormalTexturePath = findTexture({"_normal", "_norm", "_n.", "_nrm"});
        }
        
        // Specular
        if (matData.SpecularTexturePath.empty()) {
            matData.SpecularTexturePath = findTexture({"_specular", "_spec", "_s."});
        }
        
        // AO/Occlusion (including common misspelling)
        if (matData.AOTexturePath.empty()) {
            matData.AOTexturePath = findTexture({"_ao", "_occlusion", "_oclussion", "_ambient"});
        }
        
        // Roughness
        if (matData.RoughnessTexturePath.empty()) {
            matData.RoughnessTexturePath = findTexture({"_roughness", "_rough", "_r."});
        }
        
        // Metallic
        if (matData.MetallicTexturePath.empty()) {
            matData.MetallicTexturePath = findTexture({"_metallic", "_metal", "_m."});
        }
        
        // Emissive
        if (matData.EmissiveTexturePath.empty()) {
            matData.EmissiveTexturePath = findTexture({"_emissive", "_emission", "_glow"});
        }
    }

    SE_LOG_INFO("AssimpModelLoader: Processed material '{}': diffuse={}, normal={}, ao={}", 
                matData.Name,
                matData.DiffuseTexturePath.empty() ? "none" : matData.DiffuseTexturePath,
                matData.NormalTexturePath.empty() ? "none" : matData.NormalTexturePath,
                matData.AOTexturePath.empty() ? "none" : matData.AOTexturePath);

    return matData;
}

}  // namespace se
