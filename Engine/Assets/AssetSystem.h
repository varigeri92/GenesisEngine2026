#pragma once
#include "AssetData.h"
#include "../Core/Handles.h"
#include "../Systems/system.h"

namespace gns
{
    struct Material;
    struct Mesh;
    struct Texture;
}

namespace gns::assets
{
    struct AssetRequestEntry
    {
        std::string path = {};
        Handle assetHandle;
        AssetType assetType = Generic;
        bool loaded = false;
    };
    
    struct AssetCacheEntry
    {

    };

    class AssetSystem : public core::System
    {
        std::vector<assets::AssetRequestEntry> assetRequests = {};

    public:
        GNS_API void OnUpdate(float deltaTime) override;
        GNS_API void RequestAsset(const std::string& assetPath);
        GNS_API void RequestAsset(const Handle handle);

        GNS_API std::vector<LoadedObject> LoadAsset(const std::string& path);
        GNS_API std::vector<LoadedObject> LoadAsset(const std::string& path, const AssetLoadOptions& loadOptions);
        GNS_API std::vector<LoadedObject> CommitLoadedAsset(const AssetLoadResult& result);
        GNS_API gns::Mesh* EnsureMeshLoaded(Handle meshHandle);
        GNS_API gns::Material* EnsureMaterialLoaded(Handle materialHandle);
        GNS_API gns::Texture* EnsureTextureLoaded(Handle textureHandle);
        GNS_API bool ApplyImportedMaterialDefaults(gns::Material& material);

        GNS_API static Handle GetModelAssetHandle(const std::filesystem::path& sourcePath);
        GNS_API static Handle GetMeshArtifactHandle(const std::filesystem::path& sourcePath, uint32_t meshIndex);
        GNS_API static Handle GetMaterialArtifactHandle(const std::filesystem::path& sourcePath, uint32_t materialIndex);
        GNS_API static Handle GetTextureArtifactHandle(const std::filesystem::path& texturePath);

        GNS_API void Flush();
    };
}
