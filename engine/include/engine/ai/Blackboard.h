#pragma once

#include <glm.hpp>
#include <any>
#include <string>
#include <unordered_map>
#include <typeindex>

namespace se {

class Entity;

class Blackboard {
   public:
    Blackboard() = default;

    // Type-safe setters
    template <typename T>
    void Set(const std::string& key, const T& value) {
        data_[key] = value;
    }

    // Type-safe getters
    template <typename T>
    T Get(const std::string& key) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            return std::any_cast<T>(it->second);
        }
        return T{};
    }

    template <typename T>
    T Get(const std::string& key, const T& defaultValue) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (...) {
                return defaultValue;
            }
        }
        return defaultValue;
    }

    template <typename T>
    bool TryGet(const std::string& key, T& outValue) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            try {
                outValue = std::any_cast<T>(it->second);
                return true;
            } catch (...) {
                return false;
            }
        }
        return false;
    }

    bool Has(const std::string& key) const {
        return data_.find(key) != data_.end();
    }

    void Remove(const std::string& key) {
        data_.erase(key);
    }

    void Clear() {
        data_.clear();
    }

    // Common convenience setters/getters
    void SetTarget(Entity target);
    Entity GetTarget() const;
    
    void SetTargetPosition(const glm::vec3& pos) { Set("TargetPosition", pos); }
    glm::vec3 GetTargetPosition() const { return Get<glm::vec3>("TargetPosition", glm::vec3{0}); }
    
    void SetSelfPosition(const glm::vec3& pos) { Set("SelfPosition", pos); }
    glm::vec3 GetSelfPosition() const { return Get<glm::vec3>("SelfPosition", glm::vec3{0}); }
    
    void SetDistanceToTarget(float dist) { Set("DistanceToTarget", dist); }
    float GetDistanceToTarget() const { return Get<float>("DistanceToTarget", 0.0f); }
    
    void SetCanSeeTarget(bool canSee) { Set("CanSeeTarget", canSee); }
    bool GetCanSeeTarget() const { return Get<bool>("CanSeeTarget", false); }

   private:
    std::unordered_map<std::string, std::any> data_;
};

// Common blackboard keys
namespace BlackboardKeys {
    constexpr const char* Target = "Target";
    constexpr const char* TargetPosition = "TargetPosition";
    constexpr const char* SelfPosition = "SelfPosition";
    constexpr const char* DistanceToTarget = "DistanceToTarget";
    constexpr const char* CanSeeTarget = "CanSeeTarget";
    constexpr const char* LastKnownPosition = "LastKnownPosition";
    constexpr const char* HomePosition = "HomePosition";
    constexpr const char* PatrolIndex = "PatrolIndex";
}  // namespace BlackboardKeys

}  // namespace se
