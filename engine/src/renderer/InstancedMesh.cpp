#include "engine/renderer/InstancedMesh.h"

#include "engine/core/Log.h"
#include "engine/renderer/RenderCommand.h"

namespace se {

InstancedMesh::InstancedMesh(const std::shared_ptr<VertexArray>& baseVA, uint32_t maxInstances)
    : maxInstances_(maxInstances) {

    instanceBuffer_ = CreateInstanceBuffer(InstanceData::GetStride(), maxInstances, InstanceBufferUsage::Dynamic);
    if (!instanceBuffer_) {
        SE_LOG_ERROR("Failed to create instance buffer for InstancedMesh");
        return;
    }

    instancedVA_ = std::make_shared<VertexArray>();

    // Copy vertex buffers from base VA
    for (const auto& vb : baseVA->GetVertexBuffers()) {
        instancedVA_->AddVertexBuffer(vb);
    }

    // Set index buffer from base VA
    if (baseVA->GetIndexBuffer()) {
        instancedVA_->SetIndexBuffer(baseVA->GetIndexBuffer());
    }

    // Add instance buffer with layout
    instancedVA_->AddInstanceBuffer(instanceBuffer_, InstanceData::GetLayout());

    SE_LOG_INFO("Created InstancedMesh with maxInstances={}", maxInstances);
}

void InstancedMesh::SetInstances(const std::vector<InstanceData>& instances) {
    if (instances.empty()) {
        currentInstanceCount_ = 0;
        return;
    }

    uint32_t count = static_cast<uint32_t>(instances.size());
    if (count > maxInstances_) {
        SE_LOG_WARN("Instance count {} exceeds max {}, clamping", count, maxInstances_);
        count = maxInstances_;
    }

    instanceBuffer_->SetData(instances.data(), count * InstanceData::GetStride(), count);
    currentInstanceCount_ = count;
}

void InstancedMesh::UpdateInstance(uint32_t index, const InstanceData& data) {
    if (index >= currentInstanceCount_) {
        SE_LOG_WARN("Instance index {} out of range (count={})", index, currentInstanceCount_);
        return;
    }

    instanceBuffer_->SetSubData(&data, index * InstanceData::GetStride(), InstanceData::GetStride());
}

void InstancedMesh::Draw(const std::shared_ptr<Material>& material) {
    if (currentInstanceCount_ == 0) return;
    if (!material) {
        SE_LOG_WARN("InstancedMesh::Draw called with null material");
        return;
    }

    material->Bind();
    RenderCommand::DrawIndexedInstanced(instancedVA_.get(), currentInstanceCount_);
}

void InstancedMesh::DrawWithoutMaterial() {
    if (currentInstanceCount_ == 0) return;
    RenderCommand::DrawIndexedInstanced(instancedVA_.get(), currentInstanceCount_);
}

}  // namespace se
