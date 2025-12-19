#pragma once

#include <LinearMath/btIDebugDraw.h>

#include <memory>
#include <vector>

#include "engine/Camera.h"
#include "engine/Shader.h"
#include "engine/renderer/RenderCommand.h"
#include "engine/renderer/VertexArray.h"

namespace se {

class PhysicsDebugDraw : public btIDebugDraw {
   public:
    PhysicsDebugDraw();
    ~PhysicsDebugDraw();

    void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;
    void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override;
    void reportErrorWarning(const char* warningString) override;
    void draw3dText(const btVector3& location, const char* textString) override;
    void setDebugMode(int debugMode) override;
    int  getDebugMode() const override;

    void Flush(const Camera& camera);

    // Timed debug drawing
    void DrawDebugLine(const Vector3& from, const Vector3& to, const Vector3& color, float duration);
    void DrawDebugSphere(const Vector3& center, float radius, const Vector3& color, float duration, int segments = 16);
    void DrawDebugPoint(const Vector3& point, float size, const Vector3& color, float duration);

    // Update timed elements (call every frame)
    void UpdateTimedElements(float deltaTime);

   private:
    struct DebugLine {
        Vector3 From;
        Vector3 To;
        Vector3 Color;
    };

    struct TimedDebugLine {
        Vector3 From;
        Vector3 To;
        Vector3 Color;
        float   RemainingTime;
    };

    std::vector<DebugLine>      lines_;
    std::vector<TimedDebugLine> timedLines_;
    std::shared_ptr<Shader>     shader_;
    int                         debug_mode_ = 0;

    std::shared_ptr<VertexArray>  vao_;
    std::shared_ptr<VertexBuffer> vbo_;
    uint32_t                      buffer_capacity_ = 0;

    std::mutex mutex_;
};

}  // namespace se
