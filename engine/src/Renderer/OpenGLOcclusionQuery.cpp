#include "engine/renderer/OpenGLOcclusionQuery.h"

#include <glad/glad.h>

#include "engine/Log.h"

namespace se {

// ============== OpenGLOcclusionQuery ==============

OpenGLOcclusionQuery::OpenGLOcclusionQuery() {
    glGenQueries(1, &queryId_);
    if (queryId_ == 0) { SE_LOG_ERROR("Failed to create occlusion query"); }
}

OpenGLOcclusionQuery::~OpenGLOcclusionQuery() {
    if (queryId_) { glDeleteQueries(1, &queryId_); }
}

void OpenGLOcclusionQuery::Begin() {
    if (inQuery_) {
        SE_LOG_WARN("Occlusion query already active");
        return;
    }
    resultCached_ = false;
    glBeginQuery(GL_ANY_SAMPLES_PASSED, queryId_);
    inQuery_ = true;
}

void OpenGLOcclusionQuery::End() {
    if (!inQuery_) {
        SE_LOG_WARN("No active occlusion query to end");
        return;
    }
    glEndQuery(GL_ANY_SAMPLES_PASSED);
    inQuery_ = false;
}

bool OpenGLOcclusionQuery::IsResultAvailable() const {
    GLint available = GL_FALSE;
    glGetQueryObjectiv(queryId_, GL_QUERY_RESULT_AVAILABLE, &available);
    return available == GL_TRUE;
}

uint32_t OpenGLOcclusionQuery::GetResult() const {
    if (!IsResultAvailable()) {
        return cachedResult_;  // Return cached or 0
    }

    GLuint result = 0;
    glGetQueryObjectuiv(queryId_, GL_QUERY_RESULT, &result);
    cachedResult_ = result;
    resultCached_ = true;
    return result;
}

uint32_t OpenGLOcclusionQuery::GetResultBlocking() const {
    GLuint result = 0;
    glGetQueryObjectuiv(queryId_, GL_QUERY_RESULT, &result);  // Blocks until ready
    cachedResult_ = result;
    resultCached_ = true;
    return result;
}

bool OpenGLOcclusionQuery::WasVisible() const {
    return GetResult() > 0;
}

// ============== OpenGLOcclusionQueryPool ==============

OpenGLOcclusionQueryPool::OpenGLOcclusionQueryPool(uint32_t initialSize) : poolSize_(initialSize) {
    allQueries_.reserve(initialSize);
    for (uint32_t i = 0; i < initialSize; i++) {
        auto query = std::make_shared<OpenGLOcclusionQuery>();
        allQueries_.push_back(query);
        available_.push(query);
    }
    SE_LOG_INFO("Created occlusion query pool with {} queries", initialSize);
}

std::shared_ptr<IOcclusionQuery> OpenGLOcclusionQueryPool::Acquire() {
    if (available_.empty()) {
        // Grow pool
        auto query = std::make_shared<OpenGLOcclusionQuery>();
        allQueries_.push_back(query);
        poolSize_++;
        activeCount_++;
        return query;
    }

    auto query = available_.front();
    available_.pop();
    activeCount_++;
    return query;
}

void OpenGLOcclusionQueryPool::Release(std::shared_ptr<IOcclusionQuery> query) {
    available_.push(query);
    if (activeCount_ > 0) activeCount_--;
}

void OpenGLOcclusionQueryPool::ReleaseAll() {
    while (!available_.empty()) { available_.pop(); }
    for (auto& query : allQueries_) { available_.push(query); }
    activeCount_ = 0;
}

// Factory functions
std::unique_ptr<IOcclusionQuery> CreateOcclusionQuery() {
    return std::make_unique<OpenGLOcclusionQuery>();
}

std::unique_ptr<OcclusionQueryPool> CreateOcclusionQueryPool(uint32_t initialSize) {
    return std::make_unique<OpenGLOcclusionQueryPool>(initialSize);
}

}  // namespace se
