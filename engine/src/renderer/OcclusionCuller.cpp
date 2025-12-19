#include "engine/renderer/OcclusionCuller.h"

#include <gtc/matrix_transform.hpp>

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/renderer/Buffer.h"
#include "engine/renderer/GraphicsContext.h"
// #include "engine/renderer/RenderCommand.h" - Removed
#include "engine/renderer/Shader.h"

namespace {

// Minimal shader for bounding box depth testing
const char* kOcclusionVertexShader = R"(#version 330 core
layout(location = 0) in vec3 a_Position;

uniform mat4 uMVP;

void main() {
    gl_Position = uMVP * vec4(a_Position, 1.0);
}
)";

const char* kOcclusionFragmentShader = R"(#version 330 core
void main() {
    // Depth-only pass, no color output needed
}
)";

// Unit cube vertices (-0.5 to 0.5)
const float kCubeVertices[] = {
    -0.5f, -0.5f, 0.5f,  0.5f, -0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  -0.5f, 0.5f, 0.5f,
    -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f,
};

const uint32_t kCubeIndices[] = {
    0, 1, 2, 2, 3, 0,  // Front
    1, 5, 6, 6, 2, 1,  // Right
    5, 4, 7, 7, 6, 5,  // Back
    4, 0, 3, 3, 7, 4,  // Left
    3, 2, 6, 6, 7, 3,  // Top
    4, 5, 1, 1, 0, 4,  // Bottom
};

}  // namespace

namespace se {

static RHI::IDevice* GetDevice() {
    auto& app     = Application::Get();
    auto* context = app.GetWindow().GetContext();
    return context ? context->GetDevice() : nullptr;
}

OcclusionCuller::OcclusionCuller() {}

OcclusionCuller::~OcclusionCuller() {
    if (initialized_) { Shutdown(); }
}

void OcclusionCuller::Init() {
    if (initialized_) return;

    SE_LOG_INFO("Initializing OcclusionCuller");

    // Create query pool
    queryPool_ = CreateOcclusionQueryPool(256);

    // Create bounding box VAO
    boundingBoxVA_ = std::make_shared<VertexArray>();
    auto vb        = std::make_shared<VertexBuffer>(kCubeVertices, sizeof(kCubeVertices));
    vb->SetLayout({{ShaderDataType::Float3, "a_Position"}});
    boundingBoxVA_->AddVertexBuffer(vb);
    auto ib = std::make_shared<IndexBuffer>(kCubeIndices, sizeof(kCubeIndices) / sizeof(uint32_t));
    boundingBoxVA_->SetIndexBuffer(ib);

    // Create occlusion test shader
    occlusionShader_ = std::make_shared<Shader>(kOcclusionVertexShader, kOcclusionFragmentShader);
    if (!occlusionShader_ || !RHI::IsValid(occlusionShader_->GetHandle())) {
        SE_LOG_ERROR("Failed to create occlusion shader");
        return;
    }

    if (auto* device = GetDevice()) {
        RHI::PipelineDescriptor desc{};
        desc.depthStencil.depthTestEnable  = true;
        desc.depthStencil.depthWriteEnable = false;  // We check against existing depth
        desc.depthStencil.depthCompareOp   = RHI::CompareOp::LessOrEqual;
        desc.rasterizer.cullMode           = RHI::CullMode::Back;  // Or none? Box is convex.
        desc.topology                      = RHI::PrimitiveTopology::TriangleList;
        desc.blend.colorWriteMask          = RHI::ColorWriteMask::None;

        // Need vertex layout from boundingBoxVA_
        if (!boundingBoxVA_->GetVertexBuffers().empty()) {
            const auto& layout = boundingBoxVA_->GetVertexBuffers()[0]->GetLayout();
            // Need to convert se::BufferLayout to RHI::VertexLayout
            // Can use Material::GetPipeline logic duplicaton or helper?
            // Duplicating simplified logic for float3 position only
            RHI::VertexLayout vertexLayout;
            vertexLayout.stride = layout.GetStride();
            RHI::VertexAttribute attr;
            attr.location = 0;
            attr.type     = RHI::VertexAttributeType::Float3;
            attr.offset   = 0;
            vertexLayout.attributes.push_back(attr);

            occlusionPipeline_ = device->CreatePipeline(desc, occlusionShader_->GetHandle(), vertexLayout);
        }
    }

    initialized_ = true;
    SE_LOG_INFO("OcclusionCuller initialized successfully");
}

void OcclusionCuller::Shutdown() {
    if (!initialized_) return;

    SE_LOG_INFO("Shutting down OcclusionCuller");

    activeQueries_.clear();
    previousFrameVisibility_.clear();
    boundingBoxVA_.reset();
    occlusionShader_.reset();
    queryPool_.reset();

    initialized_ = false;
}

void OcclusionCuller::SetViewProjection(const Matrix4& viewProj) {
    viewProj_ = viewProj;
    frustum_.ExtractPlanes(viewProj);
}

bool OcclusionCuller::IsSphereVisible(const Vector3& center, float radius) const {
    return frustum_.IsSphereInside(center, radius);
}

bool OcclusionCuller::IsAABBVisible(const Vector3& min, const Vector3& max) const {
    return frustum_.IsBoxInside(min, max);
}

bool OcclusionCuller::WasVisibleLastFrame(uint32_t objectId) const {
    auto it = previousFrameVisibility_.find(objectId);
    if (it != previousFrameVisibility_.end()) { return it->second; }
    return true;  // Assume visible if no previous data
}

void OcclusionCuller::BeginQuery(uint32_t objectId) {
    if (!enabled_ || !queryPool_ || !initialized_) return;

    auto query = queryPool_->Acquire();
    if (query) {
        query->Begin();
        activeQueries_[objectId] = query;
        currentQueryObjectId_    = objectId;
        queriesIssued_++;
    }
}

void OcclusionCuller::RenderBoundingBox(const Vector3& center, const Vector3& halfExtents) {
    if (!boundingBoxVA_ || !occlusionShader_ || !initialized_) return;

    // Create model matrix for the bounding box
    Matrix4 model = glm::translate(Matrix4(1.0f), center);
    model         = glm::scale(model, halfExtents * 2.0f);
    Matrix4 mvp   = viewProj_ * model;

    // Use pipeline
    if (auto* device = GetDevice()) {
        if (RHI::IsValid(occlusionPipeline_)) { device->BindPipeline(occlusionPipeline_); }
    }
    // occlusionShader_->bind() is no-op.
    // Set uniforms via shader wrapper (uses RHI)
    occlusionShader_->setMat4("uMVP", mvp);

    // Disable color and depth writes - we only want to test against existing depth
    // Disable color and depth writes - we only want to test against existing depth
    // Pipeline state handles this mostly (color mask = None, depth write = false)
    // But explicit calls ensure safety if pipeline binding implementation varies.
    // Using RHI abstractions now.
    if (auto* device = GetDevice()) {
        device->SetColorWriteMask(RHI::ColorWriteMask::None);
        device->SetDepthMask(false);
    }

    RHI::DrawIndexedCommand cmd{};
    cmd.indexCount    = boundingBoxVA_->GetIndexBuffer()->GetCount();
    cmd.instanceCount = 1;
    if (auto* device = GetDevice()) device->DrawIndexed(cmd);

    // Restore state - essential because SceneRenderer layout might rely on defaults
    if (auto* device = GetDevice()) {
        device->SetColorWriteMask(RHI::ColorWriteMask::All);
        device->SetDepthMask(true);
    }
}

void OcclusionCuller::EndQuery() {
    if (!enabled_ || !initialized_) return;

    auto it = activeQueries_.find(currentQueryObjectId_);
    if (it != activeQueries_.end() && it->second) { it->second->End(); }
}

void OcclusionCuller::BeginFrame() {
    // Release all queries back to pool (but keep previousFrameVisibility_ intact!)
    for (auto& [id, query] : activeQueries_) {
        if (query) { queryPool_->Release(query); }
    }
    activeQueries_.clear();
}

void OcclusionCuller::CollectResults() {
    if (!enabled_ || !initialized_) return;

    // Create new visibility map from this frame's queries
    std::unordered_map<uint32_t, bool> newVisibility;

    for (auto& [objectId, query] : activeQueries_) {
        if (query) {
            // For best accuracy, wait for result (blocking but correct)
            bool visible            = query->GetResultBlocking() > 0;
            newVisibility[objectId] = visible;

            if (!visible) { occludedCount_++; }
        }
    }

    // Replace previous frame visibility with this frame's results
    previousFrameVisibility_ = std::move(newVisibility);
}

}  // namespace se
