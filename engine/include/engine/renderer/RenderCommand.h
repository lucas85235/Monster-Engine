#pragma once

#include <glm.hpp>

namespace se {

class VertexArray;

class RenderCommand {
   public:
    static void Init();
    static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    static void SetClearColor(const Vector4& color);
    static void Clear();

    static void DrawIndexed(const VertexArray* vertexArray, uint32_t indexCount = 0);
    static void DrawArrays(const VertexArray* vertexArray, uint32_t vertexCount);
    static void DrawLines(const VertexArray* vertexArray, uint32_t vertexCount);

    static void SetDepthTest(bool enabled);
    static void SetBlend(bool enabled);
    static void SetCullFace(bool enabled);
    static void SetWireframe(bool enabled);

   private:
    RenderCommand() = delete;
};

}  // namespace se