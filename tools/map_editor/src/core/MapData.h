#pragma once
/**
 * MapData.h - Data structures for map serialization.
 *
 * Defines:
 * - PrimitiveType/ColliderType enums for entity types
 * - MapEntityData: per-entity transform, collision, and render data
 * - MapData: container for all map entities and metadata
 *
 * File format: binary (.mstmap) with magic number and version control.
 */

#include <cstdint>
#include <string>
#include <vector>

#include "Engine.h"

namespace mst {

using se::Vector3;
using se::Vector4;
using se::Matrix4;
using se::Scope;
using se::CreateScope;

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

struct MapEntityData {
    std::string   name;
    PrimitiveType primitiveType = PrimitiveType::Cube;

    Vector3 position{0.0f, 0.0f, 0.0f};
    Vector3 rotation{0.0f, 0.0f, 0.0f};
    Vector3 scale{1.0f, 1.0f, 1.0f};
    Vector4 color{1.0f, 1.0f, 1.0f, 1.0f};

    bool         hasCollision   = false;
    ColliderType colliderType   = ColliderType::None;
    Vector3      colliderSize{1.0f, 1.0f, 1.0f};
    float        colliderRadius = 0.5f;
    float        colliderHeight = 1.0f;
    
    // Rigidbody settings
    uint8_t      rigidbodyType = 0;  // 0=Static, 1=Dynamic, 2=Kinematic
    float        mass = 1.0f;
    
    // Emissive (GI) properties
    Vector3      emissiveColor{0.0f, 0.0f, 0.0f};
    float        emissiveFactor = 0.0f;
};

// Directional light data for scene lighting
struct MapDirectionalLightData {
    Vector3 direction{0.0f, -1.0f, 0.0f};  // Light direction
    Vector3 position{0.0f, 10.0f, 10.0f};  // Light position (for shadow origin)
    Vector3 rotation{-45.0f, 0.0f, 0.0f};  // Euler rotation
    Vector3 color{1.0f, 0.98f, 0.9f};      // Light color
    float   intensity = 1.5f;              // Light intensity
    bool    castShadows = true;            // Whether light casts shadows
    bool    enabled = true;                // Whether light is active
};

struct MapData {
    static constexpr uint32_t MAGIC   = 0x4D53544D;  // "MSTM"
    static constexpr uint32_t VERSION = 4;           // Bumped for emissive support

    std::string                mapName;
    std::vector<MapEntityData> entities;

    // Player Start
    bool    hasPlayerStart = false;
    Vector3 playerStartPosition{0.0f, 0.0f, 0.0f};
    Vector3 playerStartRotation{0.0f, 0.0f, 0.0f};
    
    // Directional Light (new in version 3)
    bool                     hasDirectionalLight = true;
    MapDirectionalLightData  directionalLight;

    void Clear() {
        mapName.clear();
        entities.clear();
        hasPlayerStart = false;
        playerStartPosition = {0.0f, 0.0f, 0.0f};
        playerStartRotation = {0.0f, 0.0f, 0.0f};
        hasDirectionalLight = true;
        directionalLight = MapDirectionalLightData{};
    }
};

inline const char* PrimitiveTypeToString(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Cube: return "Cube";
        case PrimitiveType::Sphere: return "Sphere";
        case PrimitiveType::Capsule: return "Capsule";
        case PrimitiveType::Cylinder: return "Cylinder";
        case PrimitiveType::Plane: return "Plane";
        default: return "Unknown";
    }
}

inline const char* ColliderTypeToString(ColliderType type) {
    switch (type) {
        case ColliderType::None: return "None";
        case ColliderType::Box: return "Box";
        case ColliderType::Sphere: return "Sphere";
        case ColliderType::Capsule: return "Capsule";
        default: return "Unknown";
    }
}

}  // namespace mst
