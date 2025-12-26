#include "engine/utils/GLUtils.h"

#include <cstring>  // for strstr
#include "engine/core/Log.h"

namespace Renderer::Utils {

const char* GLDebugSourceToString(GLenum source) {
    switch (source) {
        case GL_DEBUG_SOURCE_API:
            return "API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            return "WINDOW SYSTEM";
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            return "SHADER COMPILER";
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            return "THIRD PARTY";
        case GL_DEBUG_SOURCE_APPLICATION:
            return "APPLICATION";
        case GL_DEBUG_SOURCE_OTHER:
        default:
            return "UNKNOWN";
    }
}

const char* GLDebugTypeToString(GLenum type) {
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            return "ERROR";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            return "DEPRECATED BEHAVIOR";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            return "UDEFINED BEHAVIOR";
        case GL_DEBUG_TYPE_PORTABILITY:
            return "PORTABILITY";
        case GL_DEBUG_TYPE_PERFORMANCE:
            return "PERFORMANCE";
        case GL_DEBUG_TYPE_OTHER:
            return "OTHER";
        case GL_DEBUG_TYPE_MARKER:
            return "MARKER";
        default:
            return "UNKNOWN";
    }
}

const char* GLDebugSeverityToString(GLenum severity) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            return "HIGH";
        case GL_DEBUG_SEVERITY_MEDIUM:
            return "MEDIUM";
        case GL_DEBUG_SEVERITY_LOW:
            return "LOW";
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            return "NOTIFICATION";
        default:
            return "UNKNOWN";
    }
}

static void GLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                            const GLchar* message, const void* userParam) {
    // TODO: Custom filters
    if (severity != GL_DEBUG_SEVERITY_MEDIUM && severity != GL_DEBUG_SEVERITY_HIGH) return;

    const char* sourceStr   = Utils::GLDebugSourceToString(source);
    const char* typeStr     = Utils::GLDebugTypeToString(type);
    const char* severityStr = Utils::GLDebugSeverityToString(severity);

    SE_LOG_INFO("[OpenGL] [{} - {} ({})]: [{}] {}", severityStr, typeStr, id, sourceStr, message);
}

void InitOpenGLDebugMessageCallback() {
    // glDebugMessageCallback requires OpenGL 4.3 or ARB_debug_output extension
    // Check for OpenGL 4.3+ or the extension before calling
    GLint major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    
    // OpenGL 4.3+ has debug output as core
    bool hasDebugOutput = (major > 4) || (major == 4 && minor >= 3);
    
    // Check for extension if not OpenGL 4.3+
    if (!hasDebugOutput) {
        const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        if (extensions && strstr(extensions, "GL_ARB_debug_output")) {
            hasDebugOutput = true;
        }
    }
    
    if (hasDebugOutput && glDebugMessageCallback) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(GLDebugCallback, nullptr);
        SE_LOG_INFO("OpenGL Debug Output enabled");
    }
}

}  // namespace Renderer::Utils