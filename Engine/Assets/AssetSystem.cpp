#include "gnspch.h"
#include "AssetSystem.h"

#include "AssetManager.h"

void gns::assets::AssetSystem::OnUpdate(float deltaTime)
{
    Flush();
}

void gns::assets::AssetSystem::RequestAsset(const std::string& assetPath)
{
}

void gns::assets::AssetSystem::RequestAsset(const Handle handle)
{
}

std::vector<gns::assets::LoadedObject> gns::assets::AssetSystem::LoadAsset(const std::string& path)
{
    return AssetManager::LoadAsset(path);
}

std::vector<gns::assets::LoadedObject> gns::assets::AssetSystem::LoadAsset(
    const std::string& path,
    const AssetLoadOptions& loadOptions)
{
    return AssetManager::LoadAsset(path, loadOptions);
}

std::vector<gns::assets::LoadedObject> gns::assets::AssetSystem::CommitLoadedAsset(
    const AssetLoadResult& result)
{
    return AssetManager::CommitLoadedAsset(result);
}

gns::Mesh* gns::assets::AssetSystem::EnsureMeshLoaded(Handle meshHandle)
{
    return AssetManager::EnsureMeshLoaded(meshHandle);
}

gns::Material* gns::assets::AssetSystem::EnsureMaterialLoaded(Handle materialHandle)
{
    return AssetManager::EnsureMaterialLoaded(materialHandle);
}

gns::Texture* gns::assets::AssetSystem::EnsureTextureLoaded(Handle textureHandle)
{
    return AssetManager::EnsureTextureLoaded(textureHandle);
}

bool gns::assets::AssetSystem::ApplyImportedMaterialDefaults(gns::Material& material)
{
    return AssetManager::ApplyImportedMaterialDefaults(material);
}

gns::Handle gns::assets::AssetSystem::GetModelAssetHandle(const std::filesystem::path& sourcePath)
{
    return AssetManager::GetModelAssetHandle(sourcePath);
}

gns::Handle gns::assets::AssetSystem::GetMeshArtifactHandle(
    const std::filesystem::path& sourcePath,
    uint32_t meshIndex)
{
    return AssetManager::GetMeshArtifactHandle(sourcePath, meshIndex);
}

gns::Handle gns::assets::AssetSystem::GetMaterialArtifactHandle(
    const std::filesystem::path& sourcePath,
    uint32_t materialIndex)
{
    return AssetManager::GetMaterialArtifactHandle(sourcePath, materialIndex);
}

gns::Handle gns::assets::AssetSystem::GetTextureArtifactHandle(const std::filesystem::path& texturePath)
{
    return AssetManager::GetTextureArtifactHandle(texturePath);
}

void gns::assets::AssetSystem::Flush()
{
    bool has_unloaded = false;
    for (auto& assetRequest : assetRequests)
    {
        if (assetRequest.loaded)
            continue;

        has_unloaded = true;
    }

    if (!has_unloaded && !assetRequests.empty())
    {
        assetRequests.clear();
        return;
    }
}
