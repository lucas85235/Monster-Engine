#pragma once

#include <memory>
#include <string>

#include "engine/renderer/Model.h"

namespace se {

class ModelLoader {
   public:
    static std::shared_ptr<Model> Load(const std::string& path);
};

}  // namespace se
