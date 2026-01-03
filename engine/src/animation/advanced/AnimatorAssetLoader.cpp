#include "engine/animation/advanced/AnimatorAssetLoader.h"

#include "engine/Log.h"

#include <fstream>
#include <chrono>

namespace se::anim {

// ==================== Utility Functions ====================

bool AnimatorAssetLoader::ReadString(std::ifstream& file, std::string& str) {
    uint32_t length = 0;
    file.read(reinterpret_cast<char*>(&length), sizeof(length));
    if (!file) return false;
    
    if (length > 0) {
        str.resize(length);
        file.read(str.data(), static_cast<std::streamsize>(length));
    } else {
        str.clear();
    }
    return file.good();
}

void AnimatorAssetLoader::WriteString(std::ofstream& file, const std::string& str) {
    uint32_t length = static_cast<uint32_t>(str.size());
    file.write(reinterpret_cast<const char*>(&length), sizeof(length));
    if (length > 0) {
        file.write(str.data(), static_cast<std::streamsize>(length));
    }
}

bool AnimatorAssetLoader::ReadFloat(std::ifstream& file, float& value) {
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return file.good();
}

void AnimatorAssetLoader::WriteFloat(std::ofstream& file, float value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

bool AnimatorAssetLoader::ReadInt(std::ifstream& file, int32_t& value) {
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return file.good();
}

void AnimatorAssetLoader::WriteInt(std::ofstream& file, int32_t value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

bool AnimatorAssetLoader::ReadBool(std::ifstream& file, bool& value) {
    uint8_t b = 0;
    file.read(reinterpret_cast<char*>(&b), sizeof(b));
    value = (b != 0);
    return file.good();
}

void AnimatorAssetLoader::WriteBool(std::ofstream& file, bool value) {
    uint8_t b = value ? 1 : 0;
    file.write(reinterpret_cast<const char*>(&b), sizeof(b));
}

// ==================== Header ====================

bool AnimatorAssetLoader::ReadHeader(std::ifstream& file, uint32_t& version) {
    uint32_t magic = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != ANIMATOR_ASSET_MAGIC) {
        return false;
    }
    
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    return file.good();
}

void AnimatorAssetLoader::WriteHeader(std::ofstream& file) {
    file.write(reinterpret_cast<const char*>(&ANIMATOR_ASSET_MAGIC), sizeof(ANIMATOR_ASSET_MAGIC));
    file.write(reinterpret_cast<const char*>(&ANIMATOR_ASSET_VERSION), sizeof(ANIMATOR_ASSET_VERSION));
}

// ==================== States ====================

bool AnimatorAssetLoader::ReadStates(std::ifstream& file, AnimatorAssetData& data, uint32_t /*version*/) {
    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    
    data.states.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
        auto& state = data.states[i];
        if (!ReadString(file, state.name)) return false;
        if (!ReadString(file, state.clipPath)) return false;
        if (!ReadString(file, state.blendSpaceName)) return false;
        if (!ReadFloat(file, state.speed)) return false;
        if (!ReadBool(file, state.loop)) return false;
        if (!ReadBool(file, state.isBlendSpace)) return false;
    }
    
    return ReadString(file, data.defaultStateName);
}

void AnimatorAssetLoader::WriteStates(std::ofstream& file, const AnimatorAssetData& data) {
    uint32_t count = static_cast<uint32_t>(data.states.size());
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    
    for (const auto& state : data.states) {
        WriteString(file, state.name);
        WriteString(file, state.clipPath);
        WriteString(file, state.blendSpaceName);
        WriteFloat(file, state.speed);
        WriteBool(file, state.loop);
        WriteBool(file, state.isBlendSpace);
    }
    
    WriteString(file, data.defaultStateName);
}

// ==================== Transitions ====================

bool AnimatorAssetLoader::ReadTransitions(std::ifstream& file, AnimatorAssetData& data, uint32_t /*version*/) {
    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    
    data.transitions.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
        auto& trans = data.transitions[i];
        if (!ReadString(file, trans.fromState)) return false;
        if (!ReadString(file, trans.toState)) return false;
        if (!ReadFloat(file, trans.duration)) return false;
        if (!ReadBool(file, trans.hasExitTime)) return false;
        if (!ReadFloat(file, trans.exitTime)) return false;
        
        // Read conditions
        uint32_t condCount = 0;
        file.read(reinterpret_cast<char*>(&condCount), sizeof(condCount));
        trans.conditions.resize(condCount);
        
        for (uint32_t j = 0; j < condCount; ++j) {
            auto& cond = trans.conditions[j];
            if (!ReadString(file, cond.parameterName)) return false;
            if (!ReadString(file, cond.compareMode)) return false;
            
            // Read threshold type and value
            uint8_t type = 0;
            file.read(reinterpret_cast<char*>(&type), sizeof(type));
            
            switch (type) {
                case 0: {
                    bool val = false;
                    ReadBool(file, val);
                    cond.threshold = val;
                    break;
                }
                case 1: {
                    float val = 0.0f;
                    ReadFloat(file, val);
                    cond.threshold = val;
                    break;
                }
                case 2: {
                    int32_t val = 0;
                    ReadInt(file, val);
                    cond.threshold = val;
                    break;
                }
            }
        }
    }
    
    return file.good();
}

void AnimatorAssetLoader::WriteTransitions(std::ofstream& file, const AnimatorAssetData& data) {
    uint32_t count = static_cast<uint32_t>(data.transitions.size());
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    
    for (const auto& trans : data.transitions) {
        WriteString(file, trans.fromState);
        WriteString(file, trans.toState);
        WriteFloat(file, trans.duration);
        WriteBool(file, trans.hasExitTime);
        WriteFloat(file, trans.exitTime);
        
        uint32_t condCount = static_cast<uint32_t>(trans.conditions.size());
        file.write(reinterpret_cast<const char*>(&condCount), sizeof(condCount));
        
        for (const auto& cond : trans.conditions) {
            WriteString(file, cond.parameterName);
            WriteString(file, cond.compareMode);
            
            if (std::holds_alternative<bool>(cond.threshold)) {
                uint8_t type = 0;
                file.write(reinterpret_cast<const char*>(&type), sizeof(type));
                WriteBool(file, std::get<bool>(cond.threshold));
            } else if (std::holds_alternative<float>(cond.threshold)) {
                uint8_t type = 1;
                file.write(reinterpret_cast<const char*>(&type), sizeof(type));
                WriteFloat(file, std::get<float>(cond.threshold));
            } else {
                uint8_t type = 2;
                file.write(reinterpret_cast<const char*>(&type), sizeof(type));
                WriteInt(file, std::get<int>(cond.threshold));
            }
        }
    }
}

// ==================== Parameters ====================

bool AnimatorAssetLoader::ReadParameters(std::ifstream& file, AnimatorAssetData& data, uint32_t /*version*/) {
    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    
    data.parameters.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
        auto& param = data.parameters[i];
        if (!ReadString(file, param.name)) return false;
        
        uint8_t type = 0;
        file.read(reinterpret_cast<char*>(&type), sizeof(type));
        param.type = static_cast<ParameterType>(type);
        
        switch (param.type) {
            case ParameterType::Bool:
            case ParameterType::Trigger: {
                bool val = false;
                ReadBool(file, val);
                param.defaultValue = val;
                break;
            }
            case ParameterType::Float: {
                float val = 0.0f;
                ReadFloat(file, val);
                param.defaultValue = val;
                break;
            }
            case ParameterType::Int: {
                int32_t val = 0;
                ReadInt(file, val);
                param.defaultValue = val;
                break;
            }
        }
    }
    
    return file.good();
}

void AnimatorAssetLoader::WriteParameters(std::ofstream& file, const AnimatorAssetData& data) {
    uint32_t count = static_cast<uint32_t>(data.parameters.size());
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    
    for (const auto& param : data.parameters) {
        WriteString(file, param.name);
        uint8_t type = static_cast<uint8_t>(param.type);
        file.write(reinterpret_cast<const char*>(&type), sizeof(type));
        
        switch (param.type) {
            case ParameterType::Bool:
            case ParameterType::Trigger:
                WriteBool(file, std::get<bool>(param.defaultValue));
                break;
            case ParameterType::Float:
                WriteFloat(file, std::get<float>(param.defaultValue));
                break;
            case ParameterType::Int:
                WriteInt(file, std::get<int>(param.defaultValue));
                break;
        }
    }
}

// ==================== BlendSpaces ====================

bool AnimatorAssetLoader::ReadBlendSpaces(std::ifstream& file, AnimatorAssetData& data, uint32_t /*version*/) {
    // 1D blend spaces
    uint32_t count1D = 0;
    file.read(reinterpret_cast<char*>(&count1D), sizeof(count1D));
    data.blendSpaces1D.resize(count1D);
    
    for (uint32_t i = 0; i < count1D; ++i) {
        auto& bs = data.blendSpaces1D[i];
        bs.is2D = false;
        if (!ReadString(file, bs.name)) return false;
        
        uint32_t sampleCount = 0;
        file.read(reinterpret_cast<char*>(&sampleCount), sizeof(sampleCount));
        bs.samples.resize(sampleCount);
        
        for (uint32_t j = 0; j < sampleCount; ++j) {
            if (!ReadString(file, bs.samples[j].clipPath)) return false;
            if (!ReadFloat(file, bs.samples[j].position.x)) return false;
            bs.samples[j].position.y = 0.0f;
        }
        
        if (!ReadFloat(file, bs.minBounds.x)) return false;
        if (!ReadFloat(file, bs.maxBounds.x)) return false;
    }
    
    // 2D blend spaces
    uint32_t count2D = 0;
    file.read(reinterpret_cast<char*>(&count2D), sizeof(count2D));
    data.blendSpaces2D.resize(count2D);
    
    for (uint32_t i = 0; i < count2D; ++i) {
        auto& bs = data.blendSpaces2D[i];
        bs.is2D = true;
        if (!ReadString(file, bs.name)) return false;
        
        uint32_t sampleCount = 0;
        file.read(reinterpret_cast<char*>(&sampleCount), sizeof(sampleCount));
        bs.samples.resize(sampleCount);
        
        for (uint32_t j = 0; j < sampleCount; ++j) {
            if (!ReadString(file, bs.samples[j].clipPath)) return false;
            if (!ReadFloat(file, bs.samples[j].position.x)) return false;
            if (!ReadFloat(file, bs.samples[j].position.y)) return false;
        }
        
        if (!ReadFloat(file, bs.minBounds.x)) return false;
        if (!ReadFloat(file, bs.minBounds.y)) return false;
        if (!ReadFloat(file, bs.maxBounds.x)) return false;
        if (!ReadFloat(file, bs.maxBounds.y)) return false;
    }
    
    return file.good();
}

void AnimatorAssetLoader::WriteBlendSpaces(std::ofstream& file, const AnimatorAssetData& data) {
    // 1D blend spaces
    uint32_t count1D = static_cast<uint32_t>(data.blendSpaces1D.size());
    file.write(reinterpret_cast<const char*>(&count1D), sizeof(count1D));
    
    for (const auto& bs : data.blendSpaces1D) {
        WriteString(file, bs.name);
        
        uint32_t sampleCount = static_cast<uint32_t>(bs.samples.size());
        file.write(reinterpret_cast<const char*>(&sampleCount), sizeof(sampleCount));
        
        for (const auto& sample : bs.samples) {
            WriteString(file, sample.clipPath);
            WriteFloat(file, sample.position.x);
        }
        
        WriteFloat(file, bs.minBounds.x);
        WriteFloat(file, bs.maxBounds.x);
    }
    
    // 2D blend spaces
    uint32_t count2D = static_cast<uint32_t>(data.blendSpaces2D.size());
    file.write(reinterpret_cast<const char*>(&count2D), sizeof(count2D));
    
    for (const auto& bs : data.blendSpaces2D) {
        WriteString(file, bs.name);
        
        uint32_t sampleCount = static_cast<uint32_t>(bs.samples.size());
        file.write(reinterpret_cast<const char*>(&sampleCount), sizeof(sampleCount));
        
        for (const auto& sample : bs.samples) {
            WriteString(file, sample.clipPath);
            WriteFloat(file, sample.position.x);
            WriteFloat(file, sample.position.y);
        }
        
        WriteFloat(file, bs.minBounds.x);
        WriteFloat(file, bs.minBounds.y);
        WriteFloat(file, bs.maxBounds.x);
        WriteFloat(file, bs.maxBounds.y);
    }
}

// ==================== Layers ====================

bool AnimatorAssetLoader::ReadLayers(std::ifstream& file, AnimatorAssetData& data, uint32_t /*version*/) {
    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    
    data.layers.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
        auto& layer = data.layers[i];
        if (!ReadString(file, layer.name)) return false;
        
        int32_t priority = 0;
        if (!ReadInt(file, priority)) return false;
        layer.priority = priority;
        
        uint8_t mode = 0;
        file.read(reinterpret_cast<char*>(&mode), sizeof(mode));
        layer.blendMode = static_cast<LayerBlendMode>(mode);
        
        if (!ReadString(file, layer.boneMaskName)) return false;
        if (!ReadFloat(file, layer.defaultWeight)) return false;
    }
    
    return file.good();
}

void AnimatorAssetLoader::WriteLayers(std::ofstream& file, const AnimatorAssetData& data) {
    uint32_t count = static_cast<uint32_t>(data.layers.size());
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    
    for (const auto& layer : data.layers) {
        WriteString(file, layer.name);
        WriteInt(file, layer.priority);
        
        uint8_t mode = static_cast<uint8_t>(layer.blendMode);
        file.write(reinterpret_cast<const char*>(&mode), sizeof(mode));
        
        WriteString(file, layer.boneMaskName);
        WriteFloat(file, layer.defaultWeight);
    }
}

// ==================== BoneMasks ====================

bool AnimatorAssetLoader::ReadBoneMasks(std::ifstream& file, AnimatorAssetData& data, uint32_t /*version*/) {
    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    
    data.boneMasks.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
        auto& mask = data.boneMasks[i];
        if (!ReadString(file, mask.name)) return false;
        
        uint32_t boneCount = 0;
        file.read(reinterpret_cast<char*>(&boneCount), sizeof(boneCount));
        mask.includedBones.resize(boneCount);
        mask.boneWeights.resize(boneCount);
        
        for (uint32_t j = 0; j < boneCount; ++j) {
            if (!ReadString(file, mask.includedBones[j])) return false;
            if (!ReadFloat(file, mask.boneWeights[j])) return false;
        }
    }
    
    return file.good();
}

void AnimatorAssetLoader::WriteBoneMasks(std::ofstream& file, const AnimatorAssetData& data) {
    uint32_t count = static_cast<uint32_t>(data.boneMasks.size());
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    
    for (const auto& mask : data.boneMasks) {
        WriteString(file, mask.name);
        
        uint32_t boneCount = static_cast<uint32_t>(mask.includedBones.size());
        file.write(reinterpret_cast<const char*>(&boneCount), sizeof(boneCount));
        
        for (size_t j = 0; j < boneCount; ++j) {
            WriteString(file, mask.includedBones[j]);
            float weight = (j < mask.boneWeights.size()) ? mask.boneWeights[j] : 1.0f;
            WriteFloat(file, weight);
        }
    }
}

// ==================== LookAt Settings ====================

bool AnimatorAssetLoader::ReadLookAtSettings(std::ifstream& file, LookAtSettings& settings, uint32_t /*version*/) {
    if (!ReadFloat(file, settings.smoothSpeed)) return false;
    if (!ReadFloat(file, settings.horizontalLimit)) return false;
    if (!ReadFloat(file, settings.verticalLimit)) return false;
    if (!ReadFloat(file, settings.deadzone)) return false;
    if (!ReadBool(file, settings.enabled)) return false;
    
    uint32_t chainCount = 0;
    file.read(reinterpret_cast<char*>(&chainCount), sizeof(chainCount));
    settings.boneChain.resize(chainCount);
    
    for (uint32_t i = 0; i < chainCount; ++i) {
        auto& bone = settings.boneChain[i];
        if (!ReadString(file, bone.boneName)) return false;
        if (!ReadFloat(file, bone.horizontalWeight)) return false;
        if (!ReadFloat(file, bone.verticalWeight)) return false;
        if (!ReadFloat(file, bone.maxHorizontalDegrees)) return false;
        if (!ReadFloat(file, bone.maxVerticalDegrees)) return false;
    }
    
    return file.good();
}

void AnimatorAssetLoader::WriteLookAtSettings(std::ofstream& file, const LookAtSettings& settings) {
    WriteFloat(file, settings.smoothSpeed);
    WriteFloat(file, settings.horizontalLimit);
    WriteFloat(file, settings.verticalLimit);
    WriteFloat(file, settings.deadzone);
    WriteBool(file, settings.enabled);
    
    uint32_t chainCount = static_cast<uint32_t>(settings.boneChain.size());
    file.write(reinterpret_cast<const char*>(&chainCount), sizeof(chainCount));
    
    for (const auto& bone : settings.boneChain) {
        WriteString(file, bone.boneName);
        WriteFloat(file, bone.horizontalWeight);
        WriteFloat(file, bone.verticalWeight);
        WriteFloat(file, bone.maxHorizontalDegrees);
        WriteFloat(file, bone.maxVerticalDegrees);
    }
}

// ==================== BodyRotation Settings ====================

bool AnimatorAssetLoader::ReadBodyRotationSettings(std::ifstream& file, BodyRotationSettings& settings, uint32_t /*version*/) {
    if (!ReadFloat(file, settings.activationThreshold)) return false;
    if (!ReadFloat(file, settings.deactivationThreshold)) return false;
    if (!ReadFloat(file, settings.deadzone)) return false;
    if (!ReadFloat(file, settings.stiffness)) return false;
    if (!ReadFloat(file, settings.damping)) return false;
    if (!ReadFloat(file, settings.maxAngularVelocity)) return false;
    if (!ReadBool(file, settings.enabled)) return false;
    
    return file.good();
}

void AnimatorAssetLoader::WriteBodyRotationSettings(std::ofstream& file, const BodyRotationSettings& settings) {
    WriteFloat(file, settings.activationThreshold);
    WriteFloat(file, settings.deactivationThreshold);
    WriteFloat(file, settings.deadzone);
    WriteFloat(file, settings.stiffness);
    WriteFloat(file, settings.damping);
    WriteFloat(file, settings.maxAngularVelocity);
    WriteBool(file, settings.enabled);
}

// ==================== Main Load/Save ====================

AnimatorLoadResult AnimatorAssetLoader::Load(const std::filesystem::path& path) {
    AnimatorLoadResult result;
    
    if (!std::filesystem::exists(path)) {
        result.errorMessage = "File not found: " + path.string();
        SE_LOG_ERROR("[AnimatorAssetLoader] {}", result.errorMessage);
        return result;
    }
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        result.errorMessage = "Failed to open file: " + path.string();
        SE_LOG_ERROR("[AnimatorAssetLoader] {}", result.errorMessage);
        return result;
    }
    
    uint32_t version = 0;
    if (!ReadHeader(file, version)) {
        result.errorMessage = "Invalid file format or corrupted header";
        SE_LOG_ERROR("[AnimatorAssetLoader] {}", result.errorMessage);
        return result;
    }
    
    if (version > ANIMATOR_ASSET_VERSION) {
        result.errorMessage = "File version " + std::to_string(version) + " is newer than supported version " + std::to_string(ANIMATOR_ASSET_VERSION);
        SE_LOG_ERROR("[AnimatorAssetLoader] {}", result.errorMessage);
        return result;
    }
    
    // Read all sections
    if (!ReadString(file, result.data.name)) { result.errorMessage = "Failed to read name"; return result; }
    if (!ReadString(file, result.data.skeletonPath)) { result.errorMessage = "Failed to read skeleton path"; return result; }
    if (!ReadStates(file, result.data, version)) { result.errorMessage = "Failed to read states"; return result; }
    if (!ReadTransitions(file, result.data, version)) { result.errorMessage = "Failed to read transitions"; return result; }
    if (!ReadParameters(file, result.data, version)) { result.errorMessage = "Failed to read parameters"; return result; }
    if (!ReadBlendSpaces(file, result.data, version)) { result.errorMessage = "Failed to read blend spaces"; return result; }
    if (!ReadLayers(file, result.data, version)) { result.errorMessage = "Failed to read layers"; return result; }
    if (!ReadBoneMasks(file, result.data, version)) { result.errorMessage = "Failed to read bone masks"; return result; }
    if (!ReadLookAtSettings(file, result.data.lookAtSettings, version)) { result.errorMessage = "Failed to read look-at settings"; return result; }
    if (!ReadBodyRotationSettings(file, result.data.bodyRotationSettings, version)) { result.errorMessage = "Failed to read body rotation settings"; return result; }
    
    result.success = true;
    SE_LOG_INFO("[AnimatorAssetLoader] Loaded '{}' from '{}'", result.data.name, path.string());
    
    return result;
}

bool AnimatorAssetLoader::Save(const std::filesystem::path& path, const AnimatorAssetData& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        SE_LOG_ERROR("[AnimatorAssetLoader] Failed to create file: {}", path.string());
        return false;
    }
    
    WriteHeader(file);
    WriteString(file, data.name);
    WriteString(file, data.skeletonPath);
    WriteStates(file, data);
    WriteTransitions(file, data);
    WriteParameters(file, data);
    WriteBlendSpaces(file, data);
    WriteLayers(file, data);
    WriteBoneMasks(file, data);
    WriteLookAtSettings(file, data.lookAtSettings);
    WriteBodyRotationSettings(file, data.bodyRotationSettings);
    
    SE_LOG_INFO("[AnimatorAssetLoader] Saved '{}' to '{}'", data.name, path.string());
    return true;
}

uint64_t AnimatorAssetLoader::GetLastModifiedTime(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return 0;
    }
    
    auto ftime = std::filesystem::last_write_time(path);
    auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(
        std::chrono::clock_cast<std::chrono::system_clock>(ftime));
    return static_cast<uint64_t>(sctp.time_since_epoch().count());
}

bool AnimatorAssetLoader::CheckModified(const std::filesystem::path& path, uint64_t lastLoadTime) {
    uint64_t currentTime = GetLastModifiedTime(path);
    return currentTime > lastLoadTime;
}

}  // namespace se::anim
