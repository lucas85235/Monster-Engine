#pragma once

#include "se_pch.h"

namespace Renderer::Utils {

const char* GLDebugSourceToString(GLenum source);
const char* GLDebugTypeToString(GLenum type);
const char* GLDebugSeverityToString(GLenum severity);

void InitOpenGLDebugMessageCallback();

} // namespace Renderer::Utils