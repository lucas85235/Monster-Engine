#include "engine/resources/Model.h"

#include "engine/Log.h"

namespace se {

Model::Model(const std::string& name) : name_(name) {
}

void Model::Draw() const {
    for (const auto& submesh : subMeshes_) {
        submesh.Draw();
    }
}

void Model::AddSubMesh(SubMesh submesh) {
    subMeshes_.push_back(std::move(submesh));
}

}  // namespace se
