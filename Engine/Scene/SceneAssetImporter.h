#pragma once

#include <cstddef>
#include <filesystem>

#include "../API/API.h"

namespace gns
{
    class RenderSystem;

    namespace assets
    {
        struct AssetLoadOptions;
    }

    struct SceneAssetImportHandle
    {
        bool IsValid() const { return handle != 0; }
        bool operator==(const SceneAssetImportHandle& other) const { return handle == other.handle; }

        size_t handle = 0;
    };

    enum class SceneAssetImportState
    {
        None,
        Loading,
        Completed,
        Failed
    };

    class SceneAssetImporter
    {
    public:
        GNS_API static SceneAssetImportHandle RequestMeshAssetIntoScene(
            const std::filesystem::path& assetPath);

        GNS_API static SceneAssetImportHandle RequestMeshAssetIntoScene(
            const std::filesystem::path& assetPath,
            const assets::AssetLoadOptions& loadOptions);

        GNS_API static SceneAssetImportState GetImportState(SceneAssetImportHandle importHandle);
        GNS_API static void ReleaseImport(SceneAssetImportHandle importHandle);
        GNS_API static void UpdatePendingImports();

        GNS_API static bool LoadMeshAssetIntoScene(
            const std::filesystem::path& assetPath,
            RenderSystem& renderSystem);

        GNS_API static bool LoadMeshAssetIntoScene(
            const std::filesystem::path& assetPath,
            const assets::AssetLoadOptions& loadOptions,
            RenderSystem& renderSystem);
    };
}
