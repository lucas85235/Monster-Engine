#include "engine/renderer/RenderCommand.h"

#include <glad/glad.h>

#include "engine/renderer/VertexArray.h"

namespace se {

void RenderCommand::Init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void RenderCommand::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    glViewport(x, y, width, height);
}

void RenderCommand::SetClearColor(const glm::vec4& color) {
    glClearColor(color.r, color.g, color.b, color.a);
}

void RenderCommand::Clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderCommand::DrawIndexed(const VertexArray* vertexArray, uint32_t indexCount) {
    vertexArray->Bind();
    uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
}

void RenderCommand::DrawArrays(const VertexArray* vertexArray, uint32_t vertexCount) {
    vertexArray->Bind();
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
}

void RenderCommand::SetDepthTest(bool enabled) {
    if (enabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

void RenderCommand::SetBlend(bool enabled) {
    if (enabled)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
}

void RenderCommand::SetCullFace(bool enabled) {
    if (enabled)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
}

void RenderCommand::SetWireframe(bool enabled) {
    if (enabled)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

}  // namespace se