#include "MousePicker.h"

#include <algorithm>
#include <cmath>
#include <gtc/matrix_transform.hpp>

#include "Engine.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/Log.h"

using namespace se;

namespace mst {

se::Entity MousePicker::Pick(const Camera& camera, se::Scene& scene,
                              float mouseX, float mouseY,
                              float viewportWidth, float viewportHeight) {
    float aspectRatio = viewportWidth / viewportHeight;
    
    Vector3 rayDir = ScreenToWorldRay(camera, mouseX, mouseY, viewportWidth, viewportHeight);
    Vector3 rayOrigin = camera.GetPosition();
    
    se::Entity closestEntity;
    float closestT = std::numeric_limits<float>::max();
    
    auto view = scene.GetAllEntitiesWith<se::NameComponent, se::TransformComponent>();
    
    for (auto entityHandle : view) {
        se::Entity entity(entityHandle, &scene);
        
        auto& name = entity.GetComponent<se::NameComponent>().Name;
        if (name == "Editor Light") continue;
        
        auto& transform = entity.GetComponent<se::TransformComponent>();
        Vector3 pos = transform.Position;
        Vector3 scale = transform.Scale;
        
        Vector3 halfSize = scale * 0.5f;
        Vector3 boxMin = pos - halfSize;
        Vector3 boxMax = pos + halfSize;
        
        float t = 0.0f;
        if (RayIntersectsAABB(rayOrigin, rayDir, boxMin, boxMax, t)) {
            if (t < closestT && t > 0.0f) {
                closestT = t;
                closestEntity = entity;
            }
        }
    }
    
    return closestEntity;
}

Vector3 MousePicker::ScreenToWorldRay(const Camera& camera,
                                        float mouseX, float mouseY,
                                        float viewportWidth, float viewportHeight) {
    float aspectRatio = viewportWidth / viewportHeight;
    
    float ndcX = (2.0f * mouseX / viewportWidth) - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY / viewportHeight);
    
    Matrix4 projection = camera.getProjectionMatrix(aspectRatio);
    Matrix4 view = camera.getViewMatrix();
    
    Matrix4 invVP = glm::inverse(projection * view);
    
    Vector4 nearPoint = invVP * Vector4(ndcX, ndcY, -1.0f, 1.0f);
    Vector4 farPoint = invVP * Vector4(ndcX, ndcY, 1.0f, 1.0f);
    
    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;
    
    Vector3 rayDir = glm::normalize(Vector3(farPoint) - Vector3(nearPoint));
    
    return rayDir;
}

bool MousePicker::RayIntersectsAABB(const Vector3& rayOrigin, const Vector3& rayDir,
                                      const Vector3& boxMin, const Vector3& boxMax, float& t) {
    float tmin = 0.0f;
    float tmax = std::numeric_limits<float>::max();
    
    for (int i = 0; i < 3; ++i) {
        if (std::abs(rayDir[i]) < 1e-8f) {
            if (rayOrigin[i] < boxMin[i] || rayOrigin[i] > boxMax[i]) {
                return false;
            }
        } else {
            float invD = 1.0f / rayDir[i];
            float t1 = (boxMin[i] - rayOrigin[i]) * invD;
            float t2 = (boxMax[i] - rayOrigin[i]) * invD;
            
            if (t1 > t2) std::swap(t1, t2);
            
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            
            if (tmin > tmax) {
                return false;
            }
        }
    }
    
    t = tmin;
    return true;
}

}  // namespace mst
