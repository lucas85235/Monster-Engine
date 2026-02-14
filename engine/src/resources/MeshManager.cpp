#include "engine/resources/MeshManager.h"

#include "engine/Log.h"
#include "engine/MeshFactory.h"
#include "engine/renderer/Buffer.h"

namespace se {
std::unordered_map<PrimitiveMeshType, std::shared_ptr<VertexArray>> MeshManager::primitiveCache_;
bool                                                                MeshManager::initialized_ = false;

void MeshManager::Init() {
    if (initialized_) {
        SE_LOG_WARN("MeshManager already initialized");
        return;
    }

    SE_LOG_INFO("Initializing MeshManager");
    primitiveCache_.clear();
    initialized_ = true;
}

void MeshManager::Shutdown() {
    if (!initialized_) return;

    SE_LOG_INFO("Shutting down MeshManager");
    ClearCache();
    initialized_ = false;
}

std::shared_ptr<VertexArray> MeshManager::CreateVertexArrayFromMesh(const Mesh& mesh) {
    // COPIE os dados para garantir que eles persistem
    std::vector<float>        vertices = mesh.getVertices();
    std::vector<unsigned int> indices  = mesh.getIndices();

    SE_LOG_INFO("Creating VertexArray from mesh: {} vertices, {} indices", vertices.size() / 9, indices.size());

    // Create vertex buffer - os dados agora estão em variáveis locais
    auto vertexBuffer = std::make_shared<VertexBuffer>(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(float)));

    // Layout: position (3) + color (3) + normal (3)
    vertexBuffer->SetLayout(
        BufferLayout({{ShaderDataType::Float3, "a_Position"}, {ShaderDataType::Float3, "a_Color"}, {ShaderDataType::Float3, "a_Normal"}}));

    // Create index buffer
    auto indexBuffer = std::make_shared<IndexBuffer>(indices.data(), static_cast<uint32_t>(indices.size()));

    // Create and setup vertex array
    auto vertexArray = std::make_shared<VertexArray>();
    vertexArray->AddVertexBuffer(vertexBuffer);
    vertexArray->SetIndexBuffer(indexBuffer);

    SE_LOG_INFO("VertexArray created successfully");
    return vertexArray;
}

std::shared_ptr<VertexArray> MeshManager::GetPrimitive(PrimitiveMeshType type) {
    if (!initialized_) {
        SE_LOG_ERROR("MeshManager not initialized!");
        return nullptr;
    }

    // Check cache
    auto it = primitiveCache_.find(type);
    if (it != primitiveCache_.end()) {
        SE_LOG_INFO("Primitive mesh found in cache");
        return it->second;
    }

    // Create and cache
    SE_LOG_INFO("Creating new primitive mesh");
    auto primitive = CreatePrimitive(type);

    if (!primitive) {
        SE_LOG_ERROR("Failed to create primitive mesh!");
        return nullptr;
    }

    primitiveCache_[type] = primitive;

    SE_LOG_INFO("Created and cached primitive mesh");
    return primitive;
}

std::shared_ptr<VertexArray> MeshManager::CreatePrimitive(PrimitiveMeshType type) {
    Mesh mesh;

    switch (type) {
        case PrimitiveMeshType::Triangle:
            SE_LOG_INFO("Creating Triangle mesh");
            mesh = MeshFactory::CreateTriangle();
            break;
        case PrimitiveMeshType::Quad:
            SE_LOG_INFO("Creating Quad mesh");
            mesh = MeshFactory::CreateQuad();
            break;
        case PrimitiveMeshType::Cube:
            SE_LOG_INFO("Creating Cube mesh");
            mesh = MeshFactory::CreateCube();
            break;
        case PrimitiveMeshType::Sphere:
            SE_LOG_INFO("Creating Sphere mesh");
            mesh = MeshFactory::CreateSphere(32, 16);
            break;
        case PrimitiveMeshType::Capsule:
            SE_LOG_INFO("Creating Capsule mesh");
            mesh = MeshFactory::CreateCapsule(0.5f, 1.0f, 16);
            break;
        case PrimitiveMeshType::Cylinder:
            SE_LOG_INFO("Creating Cylinder mesh");
            mesh = MeshFactory::CreateCylinder(0.5f, 1.0f, 16);
            break;
        default:
            SE_LOG_ERROR("Unknown primitive mesh type");
            mesh = MeshFactory::CreateCube();
            break;
    }

    return CreateVertexArrayFromMesh(mesh);
}

void MeshManager::ClearCache() {
    primitiveCache_.clear();
    SE_LOG_INFO("MeshManager cache cleared");
}
}  // namespace se
