#pragma once

#include <LinearMath/btIDebugDraw.h>
#include <vector>
#include <memory>
#include "engine/renderer/VertexArray.h"
#include "engine/renderer/RenderCommand.h"
#include "engine/Shader.h"
#include "engine/Camera.h"

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
    int getDebugMode() const override;

    void Flush(const Camera& camera);

private:
    struct DebugLine {
        Vector3 From;
        Vector3 To;
        Vector3 Color;
    };

    std::vector<DebugLine> lines_;
    std::shared_ptr<Shader> shader_;
    int debug_mode_;
    
    std::shared_ptr<VertexArray> vertex_array_;
    std::shared_ptr<VertexBuffer> vertex_buffer_;
};

} // namespace se
