#pragma once

#include <array>
#include <glm.hpp>

namespace se {

/**
 * Frustum - Represents a view frustum for culling.
 * Extracts 6 planes from a view-projection matrix.
 */
class Frustum {
   public:
    enum Plane { Left = 0, Right, Bottom, Top, Near, Far, Count };

    Frustum() = default;

    void ExtractPlanes(const Matrix4& viewProjection);

    bool IsPointInside(const Vector3& point) const;
    bool IsSphereInside(const Vector3& center, float radius) const;
    bool IsBoxInside(const Vector3& min, const Vector3& max) const;
    bool IsAABBInside(const Vector3& center, const Vector3& halfExtents) const;

   private:
    struct FrustumPlane {
        Vector3 normal;
        float   distance;

        float DistanceToPoint(const Vector3& point) const {
            return glm::dot(normal, point) + distance;
        }
    };

    std::array<FrustumPlane, Plane::Count> planes_;
};

/**
 * BoundingSphere - Simple bounding volume for culling.
 */
struct BoundingSphere {
    Vector3 center{0.0f};
    float   radius = 1.0f;

    BoundingSphere() = default;
    BoundingSphere(const Vector3& c, float r) : center(c), radius(r) {}

    bool IsInsideFrustum(const Frustum& frustum) const {
        return frustum.IsSphereInside(center, radius);
    }
};

/**
 * AABB - Axis-Aligned Bounding Box for culling.
 */
struct AABB {
    Vector3 min{0.0f};
    Vector3 max{0.0f};

    AABB() = default;
    AABB(const Vector3& minPt, const Vector3& maxPt) : min(minPt), max(maxPt) {}

    Vector3 GetCenter() const {
        return (min + max) * 0.5f;
    }
    Vector3 GetHalfExtents() const {
        return (max - min) * 0.5f;
    }

    bool IsInsideFrustum(const Frustum& frustum) const {
        return frustum.IsBoxInside(min, max);
    }

    static AABB FromCenterExtents(const Vector3& center, const Vector3& halfExtents) {
        return AABB(center - halfExtents, center + halfExtents);
    }
};

}  // namespace se
