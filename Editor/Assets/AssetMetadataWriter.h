#pragma once

#include <filesystem>

namespace gns::assets
{
    struct AssetLoadOptions;
}

namespace editor::assets
{
    bool WriteModelMetaFile(
        const std::filesystem::path& modelPath,
        const gns::assets::AssetLoadOptions& loadOptions);
}
