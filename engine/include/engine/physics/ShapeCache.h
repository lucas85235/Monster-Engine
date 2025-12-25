#pragma once

#include <btBulletCollisionCommon.h>

#include <glm.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace se {

// Key for shape lookup - based on type and dimensions
struct ShapeKey {
    enum class Type : uint8_t { Box, Sphere, Capsule };

    Type  type;
    float dim1, dim2, dim3;  // Dimensions (meaning varies by type)

    bool operator==(const ShapeKey& other) const {
        const float epsilon = 0.001f;
        return type == other.type && std::abs(dim1 - other.dim1) < epsilon &&
               std::abs(dim2 - other.dim2) < epsilon && std::abs(dim3 - other.dim3) < epsilon;
    }
};

struct ShapeKeyHash {
    size_t operator()(const ShapeKey& k) const {
        size_t h1 = std::hash<int>{}(static_cast<int>(k.type));
        size_t h2 = std::hash<float>{}(std::round(k.dim1 * 100.0f));
        size_t h3 = std::hash<float>{}(std::round(k.dim2 * 100.0f));
        size_t h4 = std::hash<float>{}(std::round(k.dim3 * 100.0f));
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }
};

class ShapeCache {
   public:
    static ShapeCache& Instance() {
        static ShapeCache instance;
        return instance;
    }

    // Get or create a box shape (half-extents)
    btCollisionShape* GetBoxShape(const glm::vec3& halfExtents) {
        ShapeKey key{ShapeKey::Type::Box, halfExtents.x, halfExtents.y, halfExtents.z};
        return GetOrCreateShape(key, [&]() {
            return new btBoxShape(btVector3(halfExtents.x, halfExtents.y, halfExtents.z));
        });
    }

    // Get or create a sphere shape
    btCollisionShape* GetSphereShape(float radius) {
        ShapeKey key{ShapeKey::Type::Sphere, radius, 0, 0};
        return GetOrCreateShape(key, [&]() { return new btSphereShape(radius); });
    }

    // Get or create a capsule shape
    btCollisionShape* GetCapsuleShape(float radius, float height) {
        ShapeKey key{ShapeKey::Type::Capsule, radius, height, 0};
        return GetOrCreateShape(key, [&]() { return new btCapsuleShape(radius, height); });
    }

    size_t GetCachedShapeCount() const {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        return shape_cache_.size();
    }

    size_t GetTotalReuses() const {
        return total_reuses_;
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        for (auto& pair : shape_cache_) { delete pair.second; }
        shape_cache_.clear();
        total_reuses_ = 0;
    }

    ~ShapeCache() {
        Clear();
    }

   private:
    ShapeCache()                             = default;
    ShapeCache(const ShapeCache&)            = delete;
    ShapeCache& operator=(const ShapeCache&) = delete;

    template <typename CreateFunc>
    btCollisionShape* GetOrCreateShape(const ShapeKey& key, CreateFunc createFunc) {
        std::lock_guard<std::mutex> lock(cache_mutex_);

        auto it = shape_cache_.find(key);
        if (it != shape_cache_.end()) {
            ++total_reuses_;
            return it->second;
        }

        btCollisionShape* shape = createFunc();
        shape_cache_[key]       = shape;
        return shape;
    }

    mutable std::mutex                                            cache_mutex_;
    std::unordered_map<ShapeKey, btCollisionShape*, ShapeKeyHash> shape_cache_;
    std::atomic<size_t>                                           total_reuses_{0};
};

}  // namespace se
