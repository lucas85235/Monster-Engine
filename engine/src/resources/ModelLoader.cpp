#include "engine/resources/ModelLoader.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <filesystem>

#include "engine/core/Log.h"
#include "engine/renderer/Material.h"
#include "engine/renderer/Texture.h"
#include "engine/resources/MaterialManager.h"
#include "engine/resources/MeshManager.h"

namespace se {

namespace {
std::string directory;

std::shared_ptr<Texture> LoadTexture(const std::string& path) {
    // Implementation note: Texture class already handles loading via stbi
    return Texture::Create(path);
}

void ProcessMesh(aiMesh* mesh, const aiScene* scene, std::shared_ptr<Model> model) {
    std::vector<float>        vertices;
    std::vector<unsigned int> indices;

    // Reserve space: 11 floats per vertex
    vertices.reserve(mesh->mNumVertices * 11);

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        // Position (3)
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);

        // Color (3) - Default to white if not present
        // Note: Shader expects colors at location 1
        if (mesh->HasVertexColors(0)) {
            vertices.push_back(mesh->mColors[0][i].r);
            vertices.push_back(mesh->mColors[0][i].g);
            vertices.push_back(mesh->mColors[0][i].b);
        } else {
            vertices.push_back(1.0f);
            vertices.push_back(1.0f);
            vertices.push_back(1.0f);
        }

        // Normal (3)
        if (mesh->HasNormals()) {
            vertices.push_back(mesh->mNormals[i].x);
            vertices.push_back(mesh->mNormals[i].y);
            vertices.push_back(mesh->mNormals[i].z);
        } else {
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);
        }

        // TexCoord (2)
        if (mesh->HasTextureCoords(0)) {
            vertices.push_back(mesh->mTextureCoords[0][i].x);
            vertices.push_back(mesh->mTextureCoords[0][i].y);
        } else {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) indices.push_back(face.mIndices[j]);
    }

    // Material
    std::shared_ptr<Material> material = nullptr;
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* aiMat = scene->mMaterials[mesh->mMaterialIndex];

        // For now, create a new material based on default shader
        // Ideally we would want to support custom shaders or parameters
        // But MaterialManager::CreateMaterial(shader) uses default shader if not specified?
        // Wait, CreateMaterial takes a shader. Use Default Material as base.

        // TODO: Create a unique material instance instead of sharing default
        // But current Material system might handle copies.
        // Let's assume we want a new instance using the default shader.
        auto shader = MaterialManager::GetDefaultMaterial()->GetShader();
        material    = std::make_shared<Material>(shader);

        // Load Diffuse Texture
        if (aiMat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString str;
            aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &str);
            std::string filename = std::string(str.C_Str());

            std::filesystem::path texPath(filename);
            std::filesystem::path fullPath;

            if (texPath.is_absolute()) {
                // If path starts with /, Assimp might be giving us an absolute path or relative to root.
                // We assume it's relative to the model directory but with a leading slash.
                // relative_path() strips the root name/directory separator if present.
                fullPath = std::filesystem::path(directory) / texPath.relative_path();
            } else {
                fullPath = std::filesystem::path(directory) / texPath;
            }

            if (std::filesystem::exists(fullPath)) {
                auto texture = LoadTexture(fullPath.string());
                if (texture) {
                    // "uTexture" or "uDiffuseMap" - check shader.
                    // Basic shader might not have texture support yet?
                    // Assuming "uTexture" or implied slot 0.
                    // Material::SetTexture takes a name and handle.
                    // If shader doesn't have sampler, this might be ignored or error.
                    // But we add it anyway.
                    material->SetTexture("uTexture", texture->GetHandle());
                }
            } else {
                SE_LOG_WARN("Texture file not found: {}", fullPath.string());
            }
        }
    } else {
        material = MaterialManager::GetDefaultMaterial();
    }

    auto meshObj = std::make_shared<Mesh>(vertices, indices);
    model->AddSubMesh(meshObj, material);
}

void ProcessNode(aiNode* node, const aiScene* scene, std::shared_ptr<Model> model) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(mesh, scene, model);
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) { ProcessNode(node->mChildren[i], scene, model); }
}
}  // namespace

std::shared_ptr<Model> ModelLoader::Load(const std::string& path) {
    Assimp::Importer importer;
    const aiScene*   scene = importer.ReadFile(path,
                                               aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs |  // OpenGL expects flipped UVs usually
                                                   aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        SE_LOG_ERROR("Assimp error: {}", importer.GetErrorString());
        return nullptr;
    }

    directory  = std::filesystem::path(path).parent_path().string();
    auto model = std::make_shared<Model>();

    ProcessNode(scene->mRootNode, scene, model);

    return model;
}

}  // namespace se
