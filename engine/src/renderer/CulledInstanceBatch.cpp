#include "engine/renderer/CulledInstanceBatch.h"

#include <gtc/matrix_transform.hpp>

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/renderer/GraphicsContext.h"

namespace se {

CulledInstanceBatch::CulledInstanceBatch(const std::shared_ptr<VertexArray>& baseVA, uint32_t maxInstances) : maxInstances_(maxInstances) {
    instanceBuffer_ = CreateInstanceBuffer(InstanceData::GetStride(), maxInstances, InstanceBufferUsage::Dynamic);
    if (!instanceBuffer_) {
        SE_LOG_ERROR("Failed to create instance buffer for CulledInstanceBatch");
        return;
    }

    instancedVA_ = std::make_shared<VertexArray>();

    for (const auto& vb : baseVA->GetVertexBuffers()) { instancedVA_->AddVertexBuffer(vb); }

    if (baseVA->GetIndexBuffer()) { instancedVA_->SetIndexBuffer(baseVA->GetIndexBuffer()); }

    instancedVA_->AddInstanceBuffer(instanceBuffer_, InstanceData::GetLayout());

    allInstances_.reserve(maxInstances);
    visibleData_.reserve(maxInstances);

    SE_LOG_INFO("Created CulledInstanceBatch with maxInstances={}", maxInstances);
}

void CulledInstanceBatch::SetInstances(const std::vector<Instance>& instances) {
    allInstances_ = instances;
    needsRebuild_ = true;
}

void CulledInstanceBatch::AddInstance(const Instance& instance) {
    if (allInstances_.size() >= maxInstances_) {
        SE_LOG_WARN("CulledInstanceBatch is full, cannot add more instances");
        return;
    }
    allInstances_.push_back(instance);
    needsRebuild_ = true;
}

void CulledInstanceBatch::ClearInstances() {
    allInstances_.clear();
    visibleCount_ = 0;
    needsRebuild_ = true;
}

void CulledInstanceBatch::CullAndUpdate(const Frustum& frustum) {
    visibleData_.clear();

    for (const auto& inst : allInstances_) {
        // Frustum cull using bounding sphere
        float scaledRadius = inst.boundingRadius * glm::max(glm::max(inst.scale.x, inst.scale.y), inst.scale.z);

        if (frustum.IsSphereInside(inst.position, scaledRadius)) {
            InstanceData data;
            data.Transform = glm::translate(Matrix4(1.0f), inst.position);
            data.Transform = glm::scale(data.Transform, inst.scale);
            data.Color     = inst.color;
            visibleData_.push_back(data);
        }
    }

    visibleCount_ = static_cast<uint32_t>(visibleData_.size());

    if (visibleCount_ > 0) { instanceBuffer_->SetData(visibleData_.data(), visibleCount_ * InstanceData::GetStride(), visibleCount_); }
}

void CulledInstanceBatch::Draw(const std::shared_ptr<Material>& material) {
    if (visibleCount_ == 0) return;
    if (!material) {
        SE_LOG_WARN("CulledInstanceBatch::Draw called with null material");
        return;
    }

    material->Bind();

    auto& app = Application::Get();
    if (auto* context = app.GetWindow().GetContext()) {
        if (auto* device = context->GetDevice()) {
            instancedVA_->Bind();

            RHI::DrawIndexedCommand cmd{};
            cmd.indexCount    = instancedVA_->GetIndexBuffer()->GetCount();
            cmd.instanceCount = visibleCount_;
            device->DrawIndexed(cmd);
        }
    }
}

void CulledInstanceBatch::DrawWithoutMaterial() {
    if (visibleCount_ == 0) return;

    auto& app = Application::Get();
    if (auto* context = app.GetWindow().GetContext()) {
        if (auto* device = context->GetDevice()) {
            instancedVA_->Bind();

            RHI::DrawIndexedCommand cmd{};
            cmd.indexCount    = instancedVA_->GetIndexBuffer()->GetCount();
            cmd.instanceCount = visibleCount_;
            device->DrawIndexed(cmd);
        }
    }
}

}  // namespace se
