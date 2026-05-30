#pragma once
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "AssetData.h"
#include "../Core/Handles.h"
#include "../JobSystem/IJob.h"
#include "../Systems/system.h"

namespace gns
{
    struct Material;
    struct Mesh;
    struct Texture;
}

namespace gns::assets
{
    struct AssetRequestHandle
    {
        bool IsValid() const { return handle != 0; }
        bool operator==(const AssetRequestHandle& other) const { return handle == other.handle; }

        size_t handle = 0;
    };

    enum class AssetRequestState
    {
        None,
        Queued,
        Loading,
        Ready,
        Failed,
        Released
    };

    struct AssetRequestEntry
    {
        AssetRequestHandle requestHandle;
        std::string path = {};
        Handle assetHandle;
        AssetType assetType = Generic;
        AssetLoadOptions loadOptions = {};
        AssetRequestState state = AssetRequestState::None;
        std::vector<LoadedObject> loadedObjects = {};
        std::string error = {};
        bool loaded = false;
    };

    struct AssetSourceBatch
    {
        std::string key = {};
        std::filesystem::path sourcePath = {};
        AssetLoadOptions loadOptions = {};
        gns::jobs::JobHandle jobHandle = {};
        std::vector<AssetRequestHandle> requests = {};
    };

    class AssetSystem : public core::System
    {
        size_t m_nextRequestHandle = 1;
        std::unordered_map<size_t, assets::AssetRequestEntry> assetRequests = {};
        std::unordered_map<std::string, assets::AssetSourceBatch> m_inFlightSourceLoads = {};
        std::unordered_map<std::string, std::vector<LoadedObject>> m_loadedSources = {};
        std::unordered_map<uint64_t, assets::AssetRequestHandle> m_queuedAssetRequests = {};

    public:
        GNS_API void OnUpdate(float deltaTime) override;
        GNS_API AssetRequestHandle RequestAsset(const std::string& assetPath);
        GNS_API AssetRequestHandle RequestAsset(const std::string& assetPath, const AssetLoadOptions& loadOptions);
        GNS_API AssetRequestHandle RequestAsset(const Handle handle);
        GNS_API void QueueAsset(const Handle handle);
        GNS_API AssetRequestState GetRequestState(AssetRequestHandle requestHandle) const;
        GNS_API bool IsRequestReady(AssetRequestHandle requestHandle) const;
        GNS_API const std::vector<LoadedObject>* GetLoadedObjects(AssetRequestHandle requestHandle) const;
        GNS_API void ReleaseRequest(AssetRequestHandle requestHandle);

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

    private:
        AssetRequestHandle CreateRequest(
            const std::filesystem::path& sourcePath,
            const AssetLoadOptions& loadOptions,
            Handle requestedAssetHandle = {});
        void CompleteSourceLoad(const std::string& sourceKey, AssetSourceBatch& batch, const AssetLoadResult& result);
    };
}

namespace std
{
    template<>
    struct hash<gns::assets::AssetRequestHandle>
    {
        size_t operator()(const gns::assets::AssetRequestHandle& requestHandle) const noexcept
        {
            return requestHandle.handle;
        }
    };
}
