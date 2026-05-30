#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "AssetData.h"
#include "../Core/Handles.h"

namespace gns
{
    struct Material;
    struct Mesh;
    struct Texture;
}

namespace gns::assets
{
    class AssetManager
    {
    public:
        static std::vector<LoadedObject> LoadAsset(const std::string& path);
        static std::vector<LoadedObject> LoadAsset(const std::string& path, const AssetLoadOptions& loadOptions);
        GNS_API static std::vector<LoadedObject> CommitLoadedAsset(const AssetLoadResult& result);
        GNS_API static gns::Mesh* EnsureMeshLoaded(Handle meshHandle);
        GNS_API static gns::Material* EnsureMaterialLoaded(Handle materialHandle);
        GNS_API static gns::Texture* EnsureTextureLoaded(Handle textureHandle);
        GNS_API static bool ApplyImportedMaterialDefaults(gns::Material& material);
        GNS_API static bool ResolveAssetSource(Handle assetHandle, AssetSourceReference& outSource);
        GNS_API static Handle GetModelAssetHandle(const std::filesystem::path& sourcePath);
        GNS_API static Handle GetMeshArtifactHandle(const std::filesystem::path& sourcePath, uint32_t meshIndex);
        GNS_API static Handle GetMaterialArtifactHandle(const std::filesystem::path& sourcePath, uint32_t materialIndex);
        GNS_API static Handle GetTextureArtifactHandle(const std::filesystem::path& texturePath);
    };
}
