#include "engine/ShaderPreprocessor.h"

#include "engine/Log.h"
#include "engine/utils/FilesHandler.h"

#include <regex>
#include <sstream>

namespace se {

std::string ShaderPreprocessor::Process(const std::string& source, 
                                        const std::filesystem::path& basePath) {
    std::unordered_set<std::string> includedFiles;
    return ProcessInternal(source, basePath, includedFiles, 0);
}

std::string ShaderPreprocessor::ProcessFile(const std::filesystem::path& filePath) {
    if (!std::filesystem::exists(filePath)) {
        SE_LOG_ERROR("ShaderPreprocessor: File not found: {}", filePath.string());
        return "";
    }
    
    std::string source = readFileToString(filePath);
    std::filesystem::path basePath = filePath.parent_path();
    
    std::unordered_set<std::string> includedFiles;
    includedFiles.insert(std::filesystem::canonical(filePath).string());
    
    return ProcessInternal(source, basePath, includedFiles, 0);
}

std::string ShaderPreprocessor::ProcessInternal(const std::string& source,
                                                 const std::filesystem::path& basePath,
                                                 std::unordered_set<std::string>& includedFiles,
                                                 int depth) {
    if (depth > MAX_INCLUDE_DEPTH) {
        SE_LOG_ERROR("ShaderPreprocessor: Maximum include depth exceeded ({})", MAX_INCLUDE_DEPTH);
        return source;
    }
    
    std::stringstream result;
    std::istringstream stream(source);
    std::string line;
    int lineNumber = 0;
    
    // Regex to match #include "filename" or #include <filename>
    std::regex includeRegex(R"(^\s*#\s*include\s*[\"<]([^\">\s]+)[\">]\s*$)");
    
    while (std::getline(stream, line)) {
        lineNumber++;
        std::smatch match;
        
        if (std::regex_match(line, match, includeRegex)) {
            std::string includeFile = match[1].str();
            
            // Resolve the include path relative to the base path
            std::filesystem::path includePath = basePath / includeFile;
            
            // Also try looking in the pbr subdirectory if not found
            if (!std::filesystem::exists(includePath)) {
                includePath = basePath / "pbr" / includeFile;
            }
            
            // Also try parent directory's pbr folder
            if (!std::filesystem::exists(includePath)) {
                includePath = basePath.parent_path() / "pbr" / includeFile;
            }
            
            if (!std::filesystem::exists(includePath)) {
                SE_LOG_ERROR("ShaderPreprocessor: Include file not found: {} (from {})", 
                             includeFile, basePath.string());
                result << "// ERROR: Include file not found: " << includeFile << "\n";
                continue;
            }
            
            // Get canonical path to detect circular includes
            std::string canonicalPath = std::filesystem::canonical(includePath).string();
            
            // Check for circular include
            if (includedFiles.find(canonicalPath) != includedFiles.end()) {
                // Already included, skip (this is valid for header guards)
                result << "// Already included: " << includeFile << "\n";
                continue;
            }
            
            includedFiles.insert(canonicalPath);
            
            // Read and process the included file
            std::string includeSource = readFileToString(includePath);
            std::filesystem::path includeBasePath = includePath.parent_path();
            
            // Add line directive for better error messages (GLSL #line support)
            result << "// --- Begin include: " << includeFile << " ---\n";
            result << ProcessInternal(includeSource, includeBasePath, includedFiles, depth + 1);
            result << "// --- End include: " << includeFile << " ---\n";
            
        } else {
            result << line << "\n";
        }
    }
    
    return result.str();
}

}  // namespace se
