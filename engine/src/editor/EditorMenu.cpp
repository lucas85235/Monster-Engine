#include "engine/editor/EditorMenu.h"

#include <imgui.h>
#include <sstream>

#include "engine/ecs/Scene.h"

namespace se {

std::map<std::string, std::vector<MenuItem>>& EditorMenu::GetRegistry() {
    static std::map<std::string, std::vector<MenuItem>> registry;
    return registry;
}

void EditorMenu::Register(MenuItem item) {
    GetRegistry()[item.Category].push_back(std::move(item));
}

void EditorMenu::Clear() {
    GetRegistry().clear();
}

namespace {

// Split "Entity/Primitives" into ["Entity", "Primitives"]
std::vector<std::string> SplitCategory(const std::string& category) {
    std::vector<std::string> parts;
    std::istringstream stream(category);
    std::string segment;
    while (std::getline(stream, segment, '/')) {
        if (!segment.empty()) {
            parts.push_back(segment);
        }
    }
    return parts;
}

// Collect unique top-level category names
std::vector<std::string> GetTopLevelCategories(
    const std::map<std::string, std::vector<MenuItem>>& registry) {
    std::vector<std::string> topLevel;
    std::map<std::string, bool> seen;

    for (auto& [category, items] : registry) {
        auto parts = SplitCategory(category);
        if (!parts.empty() && !seen[parts[0]]) {
            seen[parts[0]] = true;
            topLevel.push_back(parts[0]);
        }
    }
    return topLevel;
}

// Render items under a specific category prefix
void RenderCategoryItems(const std::map<std::string, std::vector<MenuItem>>& registry,
                         const std::string& prefix, Scene* scene, int depth = 1) {
    // Collect subcategories and direct items under this prefix
    std::map<std::string, bool> subcategories;

    for (auto& [category, items] : registry) {
        auto parts = SplitCategory(category);
        if (parts.empty()) continue;

        // Build prefix from first 'depth' parts
        std::string catPrefix;
        for (int i = 0; i < depth && i < static_cast<int>(parts.size()); ++i) {
            if (i > 0) catPrefix += "/";
            catPrefix += parts[i];
        }

        if (catPrefix != prefix) continue;

        // If category has more depth, it's a subcategory
        if (static_cast<int>(parts.size()) > depth) {
            subcategories[parts[depth]] = true;
        } else {
            // Direct items at this depth
            for (auto& item : items) {
                if (ImGui::MenuItem(item.Label.c_str(),
                                    item.Shortcut.empty() ? nullptr : item.Shortcut.c_str())) {
                    if (scene && item.Action) {
                        item.Action(*scene);
                    }
                }
            }
        }
    }

    // Render subcategories as submenus
    for (auto& [subcat, _] : subcategories) {
        if (ImGui::BeginMenu(subcat.c_str())) {
            RenderCategoryItems(registry, prefix + "/" + subcat, scene, depth + 1);
            ImGui::EndMenu();
        }
    }
}

}  // anonymous namespace

void EditorMenu::RenderMenuItems(Scene* scene) {
    auto& registry = GetRegistry();
    auto topLevel = GetTopLevelCategories(registry);

    for (auto& topCat : topLevel) {
        if (ImGui::BeginMenu(topCat.c_str())) {
            RenderCategoryItems(registry, topCat, scene);
            ImGui::EndMenu();
        }
    }
}

}  // namespace se
