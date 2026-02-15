#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <glm.hpp>

namespace se {

/**
 * IOcclusionQuery - Abstract interface for GPU occlusion queries.
 * Used to determine if an object is visible before rendering it.
 */
class IOcclusionQuery {
   public:
    virtual ~IOcclusionQuery() = default;

    virtual void Begin() = 0;
    virtual void End() = 0;

    // Check if result is available (non-blocking)
    virtual bool IsResultAvailable() const = 0;

    // Get the result (number of samples that passed depth test)
    // Returns 0 if occluded, >0 if visible
    virtual uint32_t GetResult() const = 0;

    // Get result, waiting if necessary (blocking)
    virtual uint32_t GetResultBlocking() const = 0;

    // Check if object was visible (convenience method)
    virtual bool WasVisible() const = 0;

    virtual uint32_t GetHandle() const = 0;
};

/**
 * OcclusionQueryPool - Manages a pool of occlusion queries for efficient reuse.
 */
class OcclusionQueryPool {
   public:
    virtual ~OcclusionQueryPool() = default;

    // Get an available query from the pool
    virtual std::shared_ptr<IOcclusionQuery> Acquire() = 0;

    // Release a query back to the pool
    virtual void Release(std::shared_ptr<IOcclusionQuery> query) = 0;

    // Release all queries
    virtual void ReleaseAll() = 0;

    virtual uint32_t GetPoolSize() const = 0;
    virtual uint32_t GetActiveCount() const = 0;
};

// Factory functions
std::unique_ptr<IOcclusionQuery> CreateOcclusionQuery();
std::unique_ptr<OcclusionQueryPool> CreateOcclusionQueryPool(uint32_t initialSize = 64);

}  // namespace se
