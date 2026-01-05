#include "GraphSerializer.h"
#include "EditorContext.h"

#include <fstream>
#include <sstream>
#include <engine/Log.h>

namespace GraphSerializer {

std::string SerializeToString(const EditorContext& context) {
    std::ostringstream ss;
    
    ss << "ANIMGRAPH_V1\n";
    
    const auto& nodes = context.GetNodes();
    ss << "NODES " << nodes.size() << "\n";
    for (const auto& node : nodes) {
        ss << node.id << " " << node.nodeType << " ";
        ss << node.posX << " " << node.posY << " ";
        ss << node.inputPins.size() << " " << node.outputPins.size() << " ";
        
        for (int pin : node.inputPins) ss << pin << " ";
        for (int pin : node.outputPins) ss << pin << " ";
        
        ss << node.name.size() << " " << node.name << "\n";
    }
    
    const auto& links = context.GetLinks();
    ss << "LINKS " << links.size() << "\n";
    for (const auto& link : links) {
        ss << link.id << " " << link.startPin << " " << link.endPin << "\n";
    }
    
    ss << "STATE " << context.GetPreviewTime() << " " << context.GetPlaybackSpeed() << "\n";
    
    return ss.str();
}

bool DeserializeFromString(EditorContext& context, const std::string& data) {
    std::istringstream ss(data);
    std::string header;
    ss >> header;
    
    if (header != "ANIMGRAPH_V1") {
        SE_LOG_ERROR("Invalid graph format");
        return false;
    }
    
    context.CreateNewGraph();
    auto& nodes = context.GetNodes();
    auto& links = context.GetLinks();
    nodes.clear();
    links.clear();
    
    std::string section;
    ss >> section;
    
    if (section == "NODES") {
        size_t nodeCount;
        ss >> nodeCount;
        
        for (size_t i = 0; i < nodeCount; ++i) {
            EditorNode node;
            size_t inputCount, outputCount, nameLen;
            
            ss >> node.id >> node.nodeType;
            ss >> node.posX >> node.posY;
            ss >> inputCount >> outputCount;
            
            for (size_t j = 0; j < inputCount; ++j) {
                int pin;
                ss >> pin;
                node.inputPins.push_back(pin);
            }
            
            for (size_t j = 0; j < outputCount; ++j) {
                int pin;
                ss >> pin;
                node.outputPins.push_back(pin);
            }
            
            ss >> nameLen;
            ss.ignore(1);
            node.name.resize(nameLen);
            ss.read(&node.name[0], nameLen);
            
            nodes.push_back(node);
        }
    }
    
    ss >> section;
    if (section == "LINKS") {
        size_t linkCount;
        ss >> linkCount;
        
        for (size_t i = 0; i < linkCount; ++i) {
            EditorLink link;
            ss >> link.id >> link.startPin >> link.endPin;
            links.push_back(link);
        }
    }
    
    ss >> section;
    if (section == "STATE") {
        float previewTime, playbackSpeed;
        ss >> previewTime >> playbackSpeed;
        context.SetPreviewTime(previewTime);
        context.SetPlaybackSpeed(playbackSpeed);
    }
    
    SE_LOG_INFO("Graph loaded: {} nodes, {} links", nodes.size(), links.size());
    return true;
}

bool SaveToJson(const EditorContext& context, const std::string& filepath) {
    try {
        std::string data = SerializeToString(context);
        
        std::ofstream file(filepath);
        if (!file.is_open()) {
            SE_LOG_ERROR("Failed to open file: {}", filepath);
            return false;
        }
        
        file << data;
        file.close();
        
        SE_LOG_INFO("Graph saved to: {}", filepath);
        return true;
        
    } catch (const std::exception& e) {
        SE_LOG_ERROR("Failed to save graph: {}", e.what());
        return false;
    }
}

bool LoadFromJson(EditorContext& context, const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            SE_LOG_ERROR("Failed to open file: {}", filepath);
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        
        return DeserializeFromString(context, buffer.str());
        
    } catch (const std::exception& e) {
        SE_LOG_ERROR("Failed to load graph: {}", e.what());
        return false;
    }
}

}
