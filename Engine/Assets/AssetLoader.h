#pragma once

#include "AssetData.h"
#include "../API/API.h"

namespace gns::assets
{
    class AssetLoader
    {
    public:
        GNS_API static AssetLoadResult LoadSourceAsset(const std::filesystem::path& path);
        GNS_API static AssetLoadResult LoadSourceAsset(
            const std::filesystem::path& path,
            const AssetLoadOptions& loadOptions);
    };
}
