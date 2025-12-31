#pragma once
/**
 * IMaterialAssetSerializer.h - Interface for material asset serialization.
 *
 * Defines the contract for serializing/deserializing MaterialAsset objects
 * to and from binary streams.
 */

#include <istream>
#include <ostream>

namespace se {

struct MaterialAsset;

class IMaterialAssetSerializer {
public:
    virtual ~IMaterialAssetSerializer() = default;
    
    virtual bool Serialize(const MaterialAsset& asset, std::ostream& stream) const = 0;
    
    virtual bool Deserialize(MaterialAsset& asset, std::istream& stream) const = 0;
    
    virtual uint32_t GetVersion() const = 0;
};

}  // namespace se
