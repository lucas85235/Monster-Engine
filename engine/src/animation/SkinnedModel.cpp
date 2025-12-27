#include "engine/animation/SkinnedModel.h"

#include "engine/Log.h"

namespace se {

SkinnedModel::SkinnedModel(const std::string& name) : name_(name) {
}

void SkinnedModel::Draw() const {
    for (const auto& mesh : meshes_) {
        mesh.Draw();
    }
}

void SkinnedModel::AddMesh(SkinnedMesh mesh) {
    meshes_.push_back(std::move(mesh));
}

}  // namespace se
