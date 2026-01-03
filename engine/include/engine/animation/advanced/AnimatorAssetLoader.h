#pragma once
/**
 * AnimatorAssetLoader.h - Load/save .animator asset files.
 * 
 * Binary format with versioning for animator configurations.
 * Pattern follows MapLoader for consistency.
 */

#include "engine/animation/advanced/AnimatorAsset.h"

#include <filesystem>
#include <fstream>
#include <cstdint>

namespace se::anim {

struct AnimatorLoadResult {
    bool success = false;
    std::string errorMessage;
    AnimatorAssetData data;
};

class AnimatorAssetLoader {
public:
    static AnimatorLoadResult Load(const std::filesystem::path& path);
    
    static bool Save(const std::filesystem::path& path, const AnimatorAssetData& data);
    
    static bool CheckModified(const std::filesystem::path& path, uint64_t lastLoadTime);
    
    static uint64_t GetLastModifiedTime(const std::filesystem::path& path);
    
private:
    static bool ReadString(std::ifstream& file, std::string& str);
    static void WriteString(std::ofstream& file, const std::string& str);
    
    static bool ReadFloat(std::ifstream& file, float& value);
    static void WriteFloat(std::ofstream& file, float value);
    
    static bool ReadInt(std::ifstream& file, int32_t& value);
    static void WriteInt(std::ofstream& file, int32_t value);
    
    static bool ReadBool(std::ifstream& file, bool& value);
    static void WriteBool(std::ofstream& file, bool value);
    
    static bool ReadHeader(std::ifstream& file, uint32_t& version);
    static void WriteHeader(std::ofstream& file);
    
    static bool ReadStates(std::ifstream& file, AnimatorAssetData& data, uint32_t version);
    static void WriteStates(std::ofstream& file, const AnimatorAssetData& data);
    
    static bool ReadTransitions(std::ifstream& file, AnimatorAssetData& data, uint32_t version);
    static void WriteTransitions(std::ofstream& file, const AnimatorAssetData& data);
    
    static bool ReadParameters(std::ifstream& file, AnimatorAssetData& data, uint32_t version);
    static void WriteParameters(std::ofstream& file, const AnimatorAssetData& data);
    
    static bool ReadBlendSpaces(std::ifstream& file, AnimatorAssetData& data, uint32_t version);
    static void WriteBlendSpaces(std::ofstream& file, const AnimatorAssetData& data);
    
    static bool ReadLayers(std::ifstream& file, AnimatorAssetData& data, uint32_t version);
    static void WriteLayers(std::ofstream& file, const AnimatorAssetData& data);
    
    static bool ReadBoneMasks(std::ifstream& file, AnimatorAssetData& data, uint32_t version);
    static void WriteBoneMasks(std::ofstream& file, const AnimatorAssetData& data);
    
    static bool ReadLookAtSettings(std::ifstream& file, LookAtSettings& settings, uint32_t version);
    static void WriteLookAtSettings(std::ofstream& file, const LookAtSettings& settings);
    
    static bool ReadBodyRotationSettings(std::ifstream& file, BodyRotationSettings& settings, uint32_t version);
    static void WriteBodyRotationSettings(std::ofstream& file, const BodyRotationSettings& settings);
};

}  // namespace se::anim
