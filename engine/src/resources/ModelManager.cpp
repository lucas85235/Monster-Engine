#include "engine/resources/ModelManager.h"

#include <filesystem>

#include <glad/glad.h>
#include <glm.hpp>
#include "engine/Log.h"
#include "engine/renderer/Buffer.h"
#include "engine/renderer/Material.h"
#include "engine/renderer/VertexArray.h"
#include "engine/resources/AssimpModelLoader.h"
#include "engine/resources/Model.h"
#include "engine/resources/ModelData.h"
#include "engine/resources/SubMesh.h"

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

// Vertex structure matching engine's primitive layout: Position + Color + Normal
struct EngineVertex {
    glm::vec3 Position;
    glm::vec3 Color;
    glm::vec3 Normal;
};

std::shared_ptr<Model> ModelManager::CreateModelFromData(const ModelData& data) {
    auto model = std::make_shared<Model>(data.Name);
    model->SetBoundingBox(data.Bounds);

    for (const auto& submeshData : data.SubMeshes) {
        if (submeshData.Vertices.empty()) {
            SE_LOG_WARN("ModelManager: Skipping empty submesh '{}'", submeshData.Name);
            continue;
        }

        // Convert ModelVertex to EngineVertex (matching primitive layout)
        std::vector<EngineVertex> vertices;
        vertices.reserve(submeshData.Vertices.size());
        
        for (const auto& v : submeshData.Vertices) {
            EngineVertex ev;
            ev.Position = v.Position;
            ev.Color = glm::vec3(0.7f); // Default gray color
            ev.Normal = v.Normal;
            vertices.push_back(ev);
        }

        // LearnOpenGL approach: Create VAO first, then buffers WITH VAO bound
        // This ensures correct association between VAO and its buffers
        GLuint VAO, VBO, EBO;
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        // Bind VAO first
        glBindVertexArray(VAO);
        
        // Load vertex data into VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, 
                     vertices.size() * sizeof(EngineVertex), 
                     vertices.data(), 
                     GL_STATIC_DRAW);

        // Load index data into EBO (must be done with VAO bound!)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
                     submeshData.Indices.size() * sizeof(uint32_t), 
                     submeshData.Indices.data(), 
                     GL_STATIC_DRAW);

        // Set vertex attribute pointers (must match shader layout!)
        // Position (location 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 
                              sizeof(EngineVertex), 
                              (void*)offsetof(EngineVertex, Position));
        
        // Color (location 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 
                              sizeof(EngineVertex), 
                              (void*)offsetof(EngineVertex, Color));
        
        // Normal (location 2)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 
                              sizeof(EngineVertex), 
                              (void*)offsetof(EngineVertex, Normal));

        // Unbind VAO (NOT the buffers - they stay associated with the VAO)
        glBindVertexArray(0);

        // Create a simple VertexArray wrapper that just holds the OpenGL handles
        // We need to modify VertexArray to support this or create a wrapper
        auto vertexArray = std::make_shared<VertexArray>();
        
        // Manually set the internal IDs (we need to expose this in VertexArray)
        // For now, let's create buffers the traditional way but ensure correct order
        
        // Actually, let's delete what we created and use the engine's classes properly
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        
        // Create using engine classes but in correct order:
        // 1. Create VertexArray first (this generates and binds a VAO)
        vertexArray = std::make_shared<VertexArray>();
        
        // 2. Create and add VertexBuffer (VAO must be bound, AddVertexBuffer does this)
        auto vertexBuffer = std::make_shared<VertexBuffer>(
            vertices.data(),
            static_cast<uint32_t>(vertices.size() * sizeof(EngineVertex))
        );
        vertexBuffer->SetLayout(BufferLayout({
            {ShaderDataType::Float3, "a_Position"},
            {ShaderDataType::Float3, "a_Color"},
            {ShaderDataType::Float3, "a_Normal"}
        }));
        vertexArray->AddVertexBuffer(vertexBuffer);
        
        // 3. Create and set IndexBuffer (VAO must be bound, SetIndexBuffer does this)
        auto indexBuffer = std::make_shared<IndexBuffer>(
            submeshData.Indices.data(),
            static_cast<uint32_t>(submeshData.Indices.size())
        );
        vertexArray->SetIndexBuffer(indexBuffer);
        
        // 4. Unbind VAO to prevent accidental modifications
        vertexArray->Unbind();

        // Material will be set by the user/renderer
        std::shared_ptr<Material> material = nullptr;

        SubMesh submesh(vertexArray, material, submeshData.Name);
        model->AddSubMesh(std::move(submesh));

        SE_LOG_INFO("ModelManager: Created submesh '{}' with {} vertices, {} indices",
                    submeshData.Name, vertices.size(), submeshData.Indices.size());
    }

    return model;
}

}  // namespace se
