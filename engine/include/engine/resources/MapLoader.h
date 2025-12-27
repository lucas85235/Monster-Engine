#pragma once

#include <filesystem>
#include <cstdint>

#include "Engine.h"

namespace se {

class Scene;

struct MapLoadResult {
    bool    success = false;
    bool    hasPlayerStart = false;
    Vector3 playerStartPosition{0.0f, 0.0f, 0.0f};
    Vector3 playerStartRotation{0.0f, 0.0f, 0.0f};
    size_t  entityCount = 0;
    
    // Directional light data (version 3+)
    bool    hasDirectionalLight = false;
    Vector3 lightDirection{0.0f, -1.0f, 0.0f};
    Vector3 lightPosition{0.0f, 10.0f, 10.0f};
    Vector3 lightRotation{-45.0f, 0.0f, 0.0f};
    Vector3 lightColor{1.0f, 0.98f, 0.9f};
    float   lightIntensity = 1.5f;
    bool    lightCastShadows = true;
    bool    lightEnabled = true;
};

class MapLoader {
   public:
    static MapLoadResult Load(Scene& scene, const std::filesystem::path& path);

   private:
    static constexpr uint32_t MAGIC   = 0x4D53544D;  // "MSTM"
    
    enum class PrimitiveType : uint8_t {
        Cube = 0,
        Sphere,
        Capsule,
        Cylinder,
        Plane
    };

    enum class ColliderType : uint8_t {
        None = 0,
        Box,
        Sphere,
        Capsule
    };

    struct EntityData {
        std::string   name;
        PrimitiveType primitiveType = PrimitiveType::Cube;
        Vector3       position{0.0f, 0.0f, 0.0f};
        Vector3       rotation{0.0f, 0.0f, 0.0f};
        Vector3       scale{1.0f, 1.0f, 1.0f};
        Vector4       color{1.0f, 1.0f, 1.0f, 1.0f};
        bool          hasCollision = false;
        ColliderType  colliderType = ColliderType::None;
        Vector3       colliderSize{1.0f, 1.0f, 1.0f};
        float         colliderRadius = 0.5f;
        float         colliderHeight = 1.0f;
        uint8_t       rigidbodyType = 0;  // 0=Static, 1=Dynamic, 2=Kinematic
        float         mass = 1.0f;
    };

    static bool ReadString(std::ifstream& file, std::string& str);
    static bool ReadHeader(std::ifstream& file, uint32_t& entityCount, MapLoadResult& result, uint32_t& version);
    static bool ReadEntity(std::ifstream& file, EntityData& entity);
    static void CreateSceneEntity(Scene& scene, const EntityData& data);
    static void CreateDirectionalLight(Scene& scene, const MapLoadResult& result);
};

}  // namespace se
