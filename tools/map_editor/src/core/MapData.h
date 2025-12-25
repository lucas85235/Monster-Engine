#pragma once

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
};

struct MapData {
    static constexpr uint32_t MAGIC   = 0x4D53544D;  // "MSTM"
    static constexpr uint32_t VERSION = 2;

    std::string                mapName;
    std::vector<MapEntityData> entities;

    // Player Start
    bool    hasPlayerStart = false;
    Vector3 playerStartPosition{0.0f, 0.0f, 0.0f};
    Vector3 playerStartRotation{0.0f, 0.0f, 0.0f};

    void Clear() {
        mapName.clear();
        entities.clear();
        hasPlayerStart = false;
        playerStartPosition = {0.0f, 0.0f, 0.0f};
        playerStartRotation = {0.0f, 0.0f, 0.0f};
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
