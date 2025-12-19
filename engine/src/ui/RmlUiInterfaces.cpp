#include "engine/ui/RmlUiInterfaces.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "engine/Log.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <string_view>

namespace se {

// Helper to load shader source
static std::string LoadShaderSource(const std::string& filepath) {
    std::ifstream stream(filepath);
    std::stringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

static GLuint CompileGLShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        SE_LOG_ERROR("Shader Compilation Failed: {0}", infoLog);
    }
    return shader;
}

RmlUiRenderInterface::RmlUiRenderInterface() {
    std::string vertSource = LoadShaderSource("assets/shaders/RmlUi.vert");
    std::string fragSource = LoadShaderSource("assets/shaders/RmlUi.frag");

    GLuint vert = CompileGLShader(GL_VERTEX_SHADER, vertSource);
    GLuint frag = CompileGLShader(GL_FRAGMENT_SHADER, fragSource);

    shader_program_ = glCreateProgram();
    glAttachShader(shader_program_, vert);
    glAttachShader(shader_program_, frag);
    glLinkProgram(shader_program_);

    GLint success;
    glGetProgramiv(shader_program_, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shader_program_, 512, nullptr, infoLog);
        SE_LOG_ERROR("Shader Linking Failed: {0}", infoLog);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
}

RmlUiRenderInterface::~RmlUiRenderInterface() {
    if (shader_program_) glDeleteProgram(shader_program_);
}

// ... CompileGeometry implementation (unchanged) ...
Rml::CompiledGeometryHandle RmlUiRenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) {
    // Create VAO/VBO/IBO
    GLuint vao, vbo, ibo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Rml::Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Rml::Vertex),
                          (const void*)offsetof(Rml::Vertex, position));

    // Color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Rml::Vertex),
                          (const void*)offsetof(Rml::Vertex, colour));

    // TexCoord
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Rml::Vertex),
                          (const void*)offsetof(Rml::Vertex, tex_coord));

    glBindVertexArray(0);

    struct GeometryData {
        GLuint vao, vbo, ibo;
        int    num_indices;
    };

    GeometryData* data = new GeometryData{vao, vbo, ibo, (int)indices.size()};
    return (Rml::CompiledGeometryHandle)data;
}

void RmlUiRenderInterface::RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) {
    struct GeometryData {
        GLuint vao, vbo, ibo;
        int    num_indices;
    };
    GeometryData* data = (GeometryData*)geometry;

    glUseProgram(shader_program_);

    // Set Projection
    glm::mat4 projection = glm::ortho(0.0f, (float)viewport_width_, (float)viewport_height_, 0.0f, -1.0f, 1.0f);
    GLint locProj = glGetUniformLocation(shader_program_, "u_Projection");
    glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));

    // Set Translation
    GLint locTrans = glGetUniformLocation(shader_program_, "u_Translation");
    glUniform2f(locTrans, translation.x, translation.y);

    // Set Texture
    GLint locUseTex = glGetUniformLocation(shader_program_, "u_UseTexture");
    if (texture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, (GLuint)texture);
        glUniform1i(locUseTex, 1);
        glUniform1i(glGetUniformLocation(shader_program_, "u_Texture"), 0);
    } else {
        glBindTexture(GL_TEXTURE_2D, 0);
        glUniform1i(locUseTex, 0);
    }

    glBindVertexArray(data->vao);
    glDrawElements(GL_TRIANGLES, data->num_indices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void RmlUiRenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {
    struct GeometryData {
        GLuint vao, vbo, ibo;
        int    num_indices;
    };
    GeometryData* data = (GeometryData*)geometry;
    
    glDeleteVertexArrays(1, &data->vao);
    glDeleteBuffers(1, &data->vbo);
    glDeleteBuffers(1, &data->ibo);
    delete data;
}

void RmlUiRenderInterface::EnableScissorRegion(bool enable) {
    if (enable)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);
}

void RmlUiRenderInterface::SetScissorRegion(Rml::Rectanglei region) {
    // OpenGL scissor starts at bottom-left, RmlUi is top-left.
    // We need window height to flip.
    glScissor(region.Left(), viewport_height_ - (region.Top() + region.Height()), region.Width(), region.Height()); 
}

Rml::TextureHandle RmlUiRenderInterface::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) {
    int width, height, channels;
    unsigned char* data = stbi_load(source.c_str(), &width, &height, &channels, 4);
    if (!data) return 0;

    texture_dimensions.x = width;
    texture_dimensions.y = height;

    Rml::TextureHandle handle = GenerateTexture(Rml::Span<const Rml::byte>(data, width * height * 4), texture_dimensions);
    stbi_image_free(data);
    return handle;
}

Rml::TextureHandle RmlUiRenderInterface::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) {
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, source_dimensions.x, source_dimensions.y, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, source.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return (Rml::TextureHandle)texture_id;
}

void RmlUiRenderInterface::ReleaseTexture(Rml::TextureHandle texture_handle) {
    GLuint texture_id = (GLuint)texture_handle;
    glDeleteTextures(1, &texture_id);
}

void RmlUiRenderInterface::SetTransform(const Rml::Matrix4f* transform) {
    // Not implemented for this simple version
}

void RmlUiRenderInterface::SetViewport(int width, int height) {
    viewport_width_ = width;
    viewport_height_ = height;
}


// System Interface Implementation

double RmlUiSystemInterface::GetElapsedTime() {
    return glfwGetTime();
}

bool RmlUiSystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& message) {
    switch (type) {
        case Rml::Log::LT_ALWAYS:
        case Rml::Log::LT_ERROR:
            SE_LOG_ERROR("RmlUi: {0}", message);
            break;
        case Rml::Log::LT_ASSERT:
            SE_LOG_ERROR("RmlUi Assert: {0}", message);
            break;
        case Rml::Log::LT_WARNING:
            SE_LOG_WARN("RmlUi: {0}", message);
            break;
        case Rml::Log::LT_INFO:
            SE_LOG_INFO("RmlUi: {0}", message);
            break;
        case Rml::Log::LT_DEBUG:
             SE_LOG_INFO("RmlUi [DEBUG]: {0}", message);
            break;
        case Rml::Log::LT_MAX:
            break;
    }
    return true;
}

// Font Engine Interface Implementation (Dummy)

bool RmlUiFontEngineInterface::LoadFontFace(const Rml::String& file_name, int face_index, bool fallback_face, Rml::Style::FontWeight weight) {
    return true;
}

bool RmlUiFontEngineInterface::LoadFontFace(Rml::Span<const Rml::byte> data, int face_index, const Rml::String& family, Rml::Style::FontStyle style, Rml::Style::FontWeight weight, bool fallback_face) {
    return true;
}

Rml::FontFaceHandle RmlUiFontEngineInterface::GetFontFaceHandle(const Rml::String& family, Rml::Style::FontStyle style, Rml::Style::FontWeight weight, int size) {
    return (Rml::FontFaceHandle)1; // Return a dummy handle
}

Rml::FontEffectsHandle RmlUiFontEngineInterface::PrepareFontEffects(Rml::FontFaceHandle handle, const Rml::FontEffectList& font_effects) {
    return (Rml::FontEffectsHandle)1;
}

const Rml::FontMetrics& RmlUiFontEngineInterface::GetFontMetrics(Rml::FontFaceHandle handle) {
    static Rml::FontMetrics metrics;
    metrics.size = 12;
    metrics.ascent = 10;
    metrics.descent = 2;
    metrics.line_spacing = 14;
    return metrics;
}

int RmlUiFontEngineInterface::GetStringWidth(Rml::FontFaceHandle handle, Rml::StringView string, const Rml::TextShapingContext& text_shaping_context, Rml::Character prior_character) {
    return 100; // Dummy fixed width
}

int RmlUiFontEngineInterface::GenerateString(Rml::RenderManager& render_manager, Rml::FontFaceHandle face_handle, Rml::FontEffectsHandle font_effects_handle, Rml::StringView string, Rml::Vector2f position, Rml::ColourbPremultiplied colour, float opacity, const Rml::TextShapingContext& text_shaping_context, Rml::TexturedMeshList& mesh_list) {
    return 100; // Dummy fixed width
}

int RmlUiFontEngineInterface::GetVersion(Rml::FontFaceHandle handle) {
    return 1;
}

void RmlUiFontEngineInterface::ReleaseFontResources() {
}

}  // namespace se
