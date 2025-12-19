#include "engine/renderer/RenderCommand.h"

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/renderer/GraphicsContext.h"
#include "engine/renderer/VertexArray.h"

namespace se {

static RHI::IDevice* GetDevice() {
    auto& app     = Application::Get();
    auto* context = app.GetWindow().GetContext();
    return context ? context->GetDevice() : nullptr;
}

void RenderCommand::Init() {
    // RHI Initialize handled in GraphicsContext.
    // Global state setup (like enable depth test) is now per-pipeline.
}

void RenderCommand::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    if (auto* device = GetDevice()) {
        RHI::Viewport vp;
        vp.x      = (float)x;
        vp.y      = (float)y;
        vp.width  = width;
        vp.height = height;
        device->SetViewport(vp);
    }
}

void RenderCommand::SetClearColor(const Vector4& color) {
    if (auto* device = GetDevice()) {
        RHI::ClearColor cc;
        cc.r = color.r;
        cc.g = color.g;
        cc.b = color.b;
        cc.a = color.a;
        device->SetClearColor(cc);
    }
}

void RenderCommand::Clear() {
    if (auto* device = GetDevice()) {
        device->Clear(true, true, false);  // Clear Color and Depth
    }
}

void RenderCommand::DrawIndexed(const VertexArray* vertexArray, uint32_t indexCount) {
    if (!vertexArray) return;

    // Bind handle logic is inside VertexArray::Bind() which calls device->BindVertexArray
    vertexArray->Bind();

    if (auto* device = GetDevice()) {
        uint32_t                count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
        RHI::DrawIndexedCommand cmd{};
        cmd.indexCount    = count;
        cmd.instanceCount = 1;
        cmd.firstIndex    = 0;  // TODO: Support offset in arguments?
        cmd.vertexOffset  = 0;
        cmd.firstInstance = 0;

        device->DrawIndexed(cmd);
    }
}

void RenderCommand::DrawIndexedInstanced(const VertexArray* vertexArray, uint32_t instanceCount, uint32_t indexCount) {
    if (instanceCount == 0 || !vertexArray) return;
    vertexArray->Bind();

    if (auto* device = GetDevice()) {
        uint32_t                count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
        RHI::DrawIndexedCommand cmd{};
        cmd.indexCount    = count;
        cmd.instanceCount = instanceCount;
        cmd.firstIndex    = 0;
        cmd.vertexOffset  = 0;
        cmd.firstInstance = 0;

        device->DrawIndexed(cmd);
    }
}

void RenderCommand::DrawArrays(const VertexArray* vertexArray, uint32_t vertexCount) {
    if (!vertexArray) return;
    vertexArray->Bind();

    if (auto* device = GetDevice()) {
        RHI::DrawCommand cmd{};
        cmd.vertexCount   = vertexCount;
        cmd.instanceCount = 1;
        cmd.firstVertex   = 0;
        cmd.firstInstance = 0;

        device->Draw(cmd);
    }
}

void RenderCommand::DrawArraysInstanced(const VertexArray* vertexArray, uint32_t vertexCount, uint32_t instanceCount) {
    if (instanceCount == 0 || !vertexArray) return;
    vertexArray->Bind();

    if (auto* device = GetDevice()) {
        RHI::DrawCommand cmd{};
        cmd.vertexCount   = vertexCount;
        cmd.instanceCount = instanceCount;
        cmd.firstVertex   = 0;
        cmd.firstInstance = 0;

        device->Draw(cmd);
    }
}

void RenderCommand::DrawLines(const VertexArray* vertexArray, uint32_t vertexCount) {
    // RHI Pipeline topology dictates lines vs triangles.
    // If we just call Draw(), it uses current pipeline topology.
    // Ensure pipeline is set to Lines before calling this?
    // Since RenderCommand is legacy, we might assume topology is set elsewhere or implicitly.
    // But RHI requires pipeline for topology.
    // We'll just call Draw. If pipeline topology is wrong, it won't draw lines.

    DrawArrays(vertexArray, vertexCount);
}

void RenderCommand::SetDepthTest(bool enabled) {
    // No-op. State managed by Pipeline.
}

void RenderCommand::SetBlend(bool enabled) {
    // No-op. State managed by Pipeline.
}

void RenderCommand::SetCullFace(bool enabled) {
    // No-op. State managed by Pipeline.
}

void RenderCommand::SetWireframe(bool enabled) {
    // Use PolygonMode in RHI/Pipeline if supported.
    // Current RHI minimal spec might not expose PolygonMode dynamic state yet.
}

}  // namespace se