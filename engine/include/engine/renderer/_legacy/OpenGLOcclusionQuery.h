#pragma once

#include <queue>

#include "engine/renderer/OcclusionQuery.h"

namespace se {

/**
 * OpenGL implementation of IOcclusionQuery.
 * Uses GL_ANY_SAMPLES_PASSED for efficient early-out culling.
 */
class OpenGLOcclusionQuery : public IOcclusionQuery {
   public:
    OpenGLOcclusionQuery();
    ~OpenGLOcclusionQuery() override;

    OpenGLOcclusionQuery(const OpenGLOcclusionQuery&)            = delete;
    OpenGLOcclusionQuery& operator=(const OpenGLOcclusionQuery&) = delete;

    void Begin() override;
    void End() override;

    bool     IsResultAvailable() const override;
    uint32_t GetResult() const override;
    uint32_t GetResultBlocking() const override;
    bool     WasVisible() const override;
    uint32_t GetHandle() const override {
        return queryId_;
    }

   private:
    uint32_t         queryId_      = 0;
    bool             inQuery_      = false;
    mutable bool     resultCached_ = false;
    mutable uint32_t cachedResult_ = 0;
};

/**
 * OpenGL implementation of OcclusionQueryPool.
 */
class OpenGLOcclusionQueryPool : public OcclusionQueryPool {
   public:
    OpenGLOcclusionQueryPool(uint32_t initialSize);
    ~OpenGLOcclusionQueryPool() override = default;

    std::shared_ptr<IOcclusionQuery> Acquire() override;
    void                             Release(std::shared_ptr<IOcclusionQuery> query) override;
    void                             ReleaseAll() override;

    uint32_t GetPoolSize() const override {
        return poolSize_;
    }
    uint32_t GetActiveCount() const override {
        return activeCount_;
    }

   private:
    std::queue<std::shared_ptr<IOcclusionQuery>>  available_;
    std::vector<std::shared_ptr<IOcclusionQuery>> allQueries_;
    uint32_t                                      poolSize_    = 0;
    uint32_t                                      activeCount_ = 0;
};

}  // namespace se
