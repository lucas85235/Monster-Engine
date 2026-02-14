#include "engine/renderer/Frustum.h"

namespace se {

void Frustum::ExtractPlanes(const Matrix4& vp) {
    // Left plane
    planes_[Left].normal.x = vp[0][3] + vp[0][0];
    planes_[Left].normal.y = vp[1][3] + vp[1][0];
    planes_[Left].normal.z = vp[2][3] + vp[2][0];
    planes_[Left].distance = vp[3][3] + vp[3][0];

    // Right plane
    planes_[Right].normal.x = vp[0][3] - vp[0][0];
    planes_[Right].normal.y = vp[1][3] - vp[1][0];
    planes_[Right].normal.z = vp[2][3] - vp[2][0];
    planes_[Right].distance = vp[3][3] - vp[3][0];

    // Bottom plane
    planes_[Bottom].normal.x = vp[0][3] + vp[0][1];
    planes_[Bottom].normal.y = vp[1][3] + vp[1][1];
    planes_[Bottom].normal.z = vp[2][3] + vp[2][1];
    planes_[Bottom].distance = vp[3][3] + vp[3][1];

    // Top plane
    planes_[Top].normal.x = vp[0][3] - vp[0][1];
    planes_[Top].normal.y = vp[1][3] - vp[1][1];
    planes_[Top].normal.z = vp[2][3] - vp[2][1];
    planes_[Top].distance = vp[3][3] - vp[3][1];

    // Near plane
    planes_[Near].normal.x = vp[0][3] + vp[0][2];
    planes_[Near].normal.y = vp[1][3] + vp[1][2];
    planes_[Near].normal.z = vp[2][3] + vp[2][2];
    planes_[Near].distance = vp[3][3] + vp[3][2];

    // Far plane
    planes_[Far].normal.x = vp[0][3] - vp[0][2];
    planes_[Far].normal.y = vp[1][3] - vp[1][2];
    planes_[Far].normal.z = vp[2][3] - vp[2][2];
    planes_[Far].distance = vp[3][3] - vp[3][2];

    // Normalize all planes
    for (auto& plane : planes_) {
        float length = glm::length(plane.normal);
        if (length > 0.0f) {
            plane.normal /= length;
            plane.distance /= length;
        }
    }
}

bool Frustum::IsPointInside(const Vector3& point) const {
    for (const auto& plane : planes_) {
        if (plane.DistanceToPoint(point) < 0.0f) { return false; }
    }
    return true;
}

bool Frustum::IsSphereInside(const Vector3& center, float radius) const {
    for (const auto& plane : planes_) {
        if (plane.DistanceToPoint(center) < -radius) { return false; }
    }
    return true;
}

bool Frustum::IsBoxInside(const Vector3& min, const Vector3& max) const {
    for (const auto& plane : planes_) {
        // Find the positive vertex (furthest along normal)
        Vector3 pVertex;
        pVertex.x = (plane.normal.x >= 0.0f) ? max.x : min.x;
        pVertex.y = (plane.normal.y >= 0.0f) ? max.y : min.y;
        pVertex.z = (plane.normal.z >= 0.0f) ? max.z : min.z;

        if (plane.DistanceToPoint(pVertex) < 0.0f) { return false; }
    }
    return true;
}

bool Frustum::IsAABBInside(const Vector3& center, const Vector3& halfExtents) const {
    return IsBoxInside(center - halfExtents, center + halfExtents);
}

}  // namespace se
