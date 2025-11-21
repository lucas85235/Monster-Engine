#pragma once

#include <RmlUi/Core.h>

namespace se {

class RmlUiRenderInterface : public Rml::RenderInterface {
public:
    RmlUiRenderInterface();
    ~RmlUiRenderInterface();

    void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) override;
    Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
    void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;
    void EnableScissorRegion(bool enable) override;
    void SetScissorRegion(Rml::Rectanglei region) override;
    Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override;
    void ReleaseTexture(Rml::TextureHandle texture) override;
    void SetTransform(const Rml::Matrix4f* transform) override;

    void SetViewport(int width, int height);

private:
    unsigned int shader_program_ = 0;
    int viewport_width_ = 0;
    int viewport_height_ = 0;
};

class RmlUiSystemInterface : public Rml::SystemInterface {
public:
    double GetElapsedTime() override;
    bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;
};

} // namespace se
