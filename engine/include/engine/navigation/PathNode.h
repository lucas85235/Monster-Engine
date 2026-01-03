#pragma once

#include <cstdint>
#include <limits>

namespace se {
namespace nav {

enum class NodeFlags : uint8_t {
    None       = 0,
    Walkable   = 1 << 0,
    Obstacle   = 1 << 1,
    Dynamic    = 1 << 2,  // Dynamic obstacle (can change at runtime)
    Water      = 1 << 3,  // Special terrain type
    Road       = 1 << 4,  // Preferred path
};

inline NodeFlags operator|(NodeFlags a, NodeFlags b) {
    return static_cast<NodeFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline NodeFlags operator&(NodeFlags a, NodeFlags b) {
    return static_cast<NodeFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline NodeFlags operator~(NodeFlags a) {
    return static_cast<NodeFlags>(~static_cast<uint8_t>(a));
}

inline bool HasFlag(NodeFlags flags, NodeFlags check) {
    return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(check)) != 0;
}

struct GridCoord {
    int32_t x = 0;
    int32_t z = 0;

    bool operator==(const GridCoord& other) const {
        return x == other.x && z == other.z;
    }

    bool operator!=(const GridCoord& other) const {
        return !(*this == other);
    }
};

struct PathNode {
    GridCoord coord;
    float     gCost   = std::numeric_limits<float>::max();  // Cost from start
    float     hCost   = 0.0f;                               // Heuristic to goal
    NodeFlags flags   = NodeFlags::Walkable;
    float     penalty = 0.0f;  // Additional movement cost
    int32_t   parentIndex = -1;

    float GetFCost() const {
        return gCost + hCost;
    }

    bool IsWalkable() const {
        return HasFlag(flags, NodeFlags::Walkable) && !HasFlag(flags, NodeFlags::Obstacle);
    }

    void Reset() {
        gCost       = std::numeric_limits<float>::max();
        hCost       = 0.0f;
        parentIndex = -1;
    }
};

}  // namespace nav
}  // namespace se

// Hash for GridCoord (for use in unordered containers)
namespace std {
template <>
struct hash<se::nav::GridCoord> {
    size_t operator()(const se::nav::GridCoord& coord) const {
        return hash<int64_t>()(static_cast<int64_t>(coord.x) << 32 | 
                               static_cast<uint32_t>(coord.z));
    }
};
}  // namespace std
