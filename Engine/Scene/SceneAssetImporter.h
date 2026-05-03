#pragma once

#include <filesystem>

#include "../API/API.h"

namespace gns
{
    class RenderSystem;

    namespace assets
    {
        struct AssetLoadOptions;
    }

    class SceneAssetImporter
    {
    public:
        GNS_API static bool LoadMeshAssetIntoScene(
            const std::filesystem::path& assetPath,
            RenderSystem& renderSystem);

        GNS_API static bool LoadMeshAssetIntoScene(
            const std::filesystem::path& assetPath,
            const assets::AssetLoadOptions& loadOptions,
            RenderSystem& renderSystem);
    };
}
