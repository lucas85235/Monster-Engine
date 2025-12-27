#pragma once

#include <memory>
#include <string>

namespace se {

struct ModelData;

class IModelLoader {
   public:
    virtual ~IModelLoader() = default;

    virtual std::unique_ptr<ModelData> Load(const std::string& path) = 0;
    virtual bool SupportsFormat(const std::string& extension) const = 0;
};

}  // namespace se
