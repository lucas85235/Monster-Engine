#pragma once

#include <string>

class EditorContext;

namespace GraphSerializer {
    bool SaveToJson(const EditorContext& context, const std::string& filepath);
    bool LoadFromJson(EditorContext& context, const std::string& filepath);
    
    std::string SerializeToString(const EditorContext& context);
    bool DeserializeFromString(EditorContext& context, const std::string& jsonStr);
}
