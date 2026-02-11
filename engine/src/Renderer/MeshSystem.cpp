#include "engine/renderer/MeshSystem.h"

#include "engine/renderer/MeshData.h"
#include "engine/renderer/MaterialHandle.h"

#include <filament/Engine.h>
#include <filament/Scene.h>
#include <filament/RenderableManager.h>
#include <filament/TransformManager.h>
#include <filament/VertexBuffer.h>
#include <filament/IndexBuffer.h>
#include <filament/MaterialInstance.h>
#include <filament/Box.h>

#include <utils/EntityManager.h>

#include <math/mat4.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <math/norm.h>

#include <spdlog/spdlog.h>

#include <cstring>

namespace se {

namespace {

/**
 * Pack a normal + tangent into a quaternion (snorm16 format)
 * as expected by Filament's TANGENTS vertex attribute.
 */
filament::math::short4 packTangentFrame(const float normal[3], const float tangent[3]) {
    using namespace filament::math;

    float3 n = normalize(float3{normal[0], normal[1], normal[2]});
    float3 t = normalize(float3{tangent[0], tangent[1], tangent[2]});

    // Ensure tangent is orthogonal to normal
    t = normalize(t - dot(t, n) * n);

    // Compute bitangent
    float3 b = cross(n, t);

    // Convert rotation matrix [t, b, n] to quaternion
    // Using standard Shepperd's method for numerical stability
    float m00 = t.x, m01 = b.x, m02 = n.x;
    float m10 = t.y, m11 = b.y, m12 = n.y;
    float m20 = t.z, m21 = b.z, m22 = n.z;

    float trace = m00 + m11 + m22;
    float qx, qy, qz, qw;

    if (trace > 0.0f) {
        float s = 0.5f / std::sqrt(trace + 1.0f);
        qw = 0.25f / s;
        qx = (m21 - m12) * s;
        qy = (m02 - m20) * s;
        qz = (m10 - m01) * s;
    } else if (m00 > m11 && m00 > m22) {
        float s = 2.0f * std::sqrt(1.0f + m00 - m11 - m22);
        qw = (m21 - m12) / s;
        qx = 0.25f * s;
        qy = (m01 + m10) / s;
        qz = (m02 + m20) / s;
    } else if (m11 > m22) {
        float s = 2.0f * std::sqrt(1.0f + m11 - m00 - m22);
        qw = (m02 - m20) / s;
        qx = (m01 + m10) / s;
        qy = 0.25f * s;
        qz = (m12 + m21) / s;
    } else {
        float s = 2.0f * std::sqrt(1.0f + m22 - m00 - m11);
        qw = (m10 - m01) / s;
        qx = (m02 + m20) / s;
        qy = (m12 + m21) / s;
        qz = 0.25f * s;
    }

    // Ensure w is positive (Filament convention)
    if (qw < 0.0f) {
        qx = -qx; qy = -qy; qz = -qz; qw = -qw;
    }

    // Ensure w is not zero (would break tangent frame)
    constexpr float kMinW = 1.0f / 32767.0f;
    if (qw < kMinW) {
        qw = kMinW;
    }

    return filament::math::packSnorm16(float4{qx, qy, qz, qw});
}

} // anonymous namespace

MeshSystem::~MeshSystem() {
    Shutdown();
}

void MeshSystem::Init(filament::Engine* engine, filament::Scene* scene) {
    if (engine_) {
        spdlog::warn("MeshSystem::Init called but already initialized. Ignoring.");
        return;
    }
    engine_ = engine;
    scene_  = scene;
    spdlog::info("MeshSystem initialized.");
}

void MeshSystem::Shutdown() {
    if (!engine_) return;

    for (auto& r : renderables_) {
        if (r.entity) {
            scene_->remove(*r.entity);
            engine_->destroy(*r.entity);
            utils::EntityManager::get().destroy(*r.entity);
            delete r.entity;
        }
        if (r.vertexBuffer) engine_->destroy(r.vertexBuffer);
        if (r.indexBuffer)  engine_->destroy(r.indexBuffer);
    }
    renderables_.clear();

    engine_ = nullptr;
    scene_  = nullptr;
    spdlog::info("MeshSystem shut down.");
}

RenderableHandle MeshSystem::CreateRenderable(const MeshData& meshData,
                                               const MaterialHandle& material,
                                               bool castShadows) {
    if (!engine_ || !scene_) {
        spdlog::error("MeshSystem::CreateRenderable called before Init()!");
        return {};
    }

    if (!meshData.IsValid()) {
        spdlog::error("MeshSystem::CreateRenderable called with invalid MeshData!");
        return {};
    }

    if (!material.IsValid()) {
        spdlog::error("MeshSystem::CreateRenderable called with invalid MaterialHandle!");
        return {};
    }

    const size_t vertexCount = meshData.GetVertexCount();
    const size_t indexCount  = meshData.GetIndexCount();

    // --- Create tangent frame data (packed quaternions for Filament) ---
    std::vector<filament::math::short4> tangents(vertexCount);
    for (size_t i = 0; i < vertexCount; ++i) {
        tangents[i] = packTangentFrame(meshData.vertices[i].normal, meshData.vertices[i].tangent);
    }

    // --- Create VertexBuffer ---
    auto* vb = filament::VertexBuffer::Builder()
        .vertexCount(static_cast<uint32_t>(vertexCount))
        .bufferCount(3) // positions, tangents, uvs
        .attribute(filament::VertexAttribute::POSITION,  0, filament::VertexBuffer::AttributeType::FLOAT3)
        .attribute(filament::VertexAttribute::TANGENTS,  1, filament::VertexBuffer::AttributeType::SHORT4)
        .normalized(filament::VertexAttribute::TANGENTS)
        .attribute(filament::VertexAttribute::UV0,       2, filament::VertexBuffer::AttributeType::FLOAT2)
        .build(*engine_);

    // Upload position data
    {
        std::vector<filament::math::float3> positions(vertexCount);
        for (size_t i = 0; i < vertexCount; ++i) {
            positions[i] = {
                meshData.vertices[i].position[0],
                meshData.vertices[i].position[1],
                meshData.vertices[i].position[2]
            };
        }
        auto* posData = new filament::math::float3[vertexCount];
        std::memcpy(posData, positions.data(), vertexCount * sizeof(filament::math::float3));
        vb->setBufferAt(*engine_, 0,
            filament::VertexBuffer::BufferDescriptor(
                posData, vertexCount * sizeof(filament::math::float3),
                [](void* buffer, size_t, void*) { delete[] static_cast<filament::math::float3*>(buffer); }
            ));
    }

    // Upload tangent data
    {
        auto* tanData = new filament::math::short4[vertexCount];
        std::memcpy(tanData, tangents.data(), vertexCount * sizeof(filament::math::short4));
        vb->setBufferAt(*engine_, 1,
            filament::VertexBuffer::BufferDescriptor(
                tanData, vertexCount * sizeof(filament::math::short4),
                [](void* buffer, size_t, void*) { delete[] static_cast<filament::math::short4*>(buffer); }
            ));
    }

    // Upload UV data
    {
        std::vector<filament::math::float2> uvs(vertexCount);
        for (size_t i = 0; i < vertexCount; ++i) {
            uvs[i] = {meshData.vertices[i].uv[0], meshData.vertices[i].uv[1]};
        }
        auto* uvData = new filament::math::float2[vertexCount];
        std::memcpy(uvData, uvs.data(), vertexCount * sizeof(filament::math::float2));
        vb->setBufferAt(*engine_, 2,
            filament::VertexBuffer::BufferDescriptor(
                uvData, vertexCount * sizeof(filament::math::float2),
                [](void* buffer, size_t, void*) { delete[] static_cast<filament::math::float2*>(buffer); }
            ));
    }

    // --- Create IndexBuffer ---
    auto* ib = filament::IndexBuffer::Builder()
        .indexCount(static_cast<uint32_t>(indexCount))
        .bufferType(filament::IndexBuffer::IndexType::UINT)
        .build(*engine_);

    {
        auto* idxData = new uint32_t[indexCount];
        std::memcpy(idxData, meshData.indices.data(), indexCount * sizeof(uint32_t));
        ib->setBuffer(*engine_,
            filament::IndexBuffer::BufferDescriptor(
                idxData, indexCount * sizeof(uint32_t),
                [](void* buffer, size_t, void*) { delete[] static_cast<uint32_t*>(buffer); }
            ));
    }

    // --- Compute bounding box ---
    filament::math::float3 minBound{FLT_MAX, FLT_MAX, FLT_MAX};
    filament::math::float3 maxBound{-FLT_MAX, -FLT_MAX, -FLT_MAX};
    for (size_t i = 0; i < vertexCount; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (meshData.vertices[i].position[j] < minBound[j])
                minBound[j] = meshData.vertices[i].position[j];
            if (meshData.vertices[i].position[j] > maxBound[j])
                maxBound[j] = meshData.vertices[i].position[j];
        }
    }
    filament::math::float3 center = (minBound + maxBound) * 0.5f;
    filament::math::float3 halfExtent = (maxBound - minBound) * 0.5f;

    // --- Create Filament entity and renderable ---
    auto& em = utils::EntityManager::get();
    auto entity = em.create();

    filament::RenderableManager::Builder(1)
        .geometry(0, filament::RenderableManager::PrimitiveType::TRIANGLES, vb, ib)
        .material(0, material.GetNative())
        .boundingBox({{center.x - halfExtent.x, center.y - halfExtent.y, center.z - halfExtent.z},
                      {center.x + halfExtent.x, center.y + halfExtent.y, center.z + halfExtent.z}})
        .castShadows(castShadows)
        .receiveShadows(true)
        .build(*engine_, entity);

    scene_->addEntity(entity);

    // Store managed renderable
    ManagedRenderable managed;
    managed.entity       = new utils::Entity(entity);
    managed.vertexBuffer = vb;
    managed.indexBuffer  = ib;
    renderables_.push_back(managed);

    RenderableHandle handle;
    handle.filamentEntity = managed.entity;

    spdlog::debug("Created renderable '{}' with {} vertices, {} indices.",
                  meshData.name, vertexCount, indexCount);

    return handle;
}

void MeshSystem::DestroyRenderable(RenderableHandle& handle) {
    if (!handle.IsValid() || !engine_) return;

    for (auto it = renderables_.begin(); it != renderables_.end(); ++it) {
        if (it->entity == handle.filamentEntity) {
            scene_->remove(*it->entity);
            engine_->destroy(*it->entity);
            utils::EntityManager::get().destroy(*it->entity);
            delete it->entity;

            if (it->vertexBuffer) engine_->destroy(it->vertexBuffer);
            if (it->indexBuffer)  engine_->destroy(it->indexBuffer);

            renderables_.erase(it);
            break;
        }
    }

    handle.filamentEntity = nullptr;
}

void MeshSystem::SetTransform(const RenderableHandle& handle, const float* transform) {
    if (!handle.IsValid() || !engine_) return;

    auto& tcm = engine_->getTransformManager();
    auto ti = tcm.getInstance(*handle.filamentEntity);

    // Convert from column-major float[16] to Filament mat4f
    filament::math::mat4f mat;
    std::memcpy(&mat, transform, sizeof(float) * 16);
    tcm.setTransform(ti, mat);
}

} // namespace se
