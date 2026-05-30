#include "gnspch.h"
#include "AssetSystem.h"

#include "AssetLoader.h"
#include "AssetManager.h"
#include "../JobSystem/JobSystem.h"
#include "../Utils/Path.h"

namespace
{
    struct AssetLoadJob : public gns::jobs::IJob
    {
        AssetLoadJob(
            std::filesystem::path sourcePath,
            gns::assets::AssetLoadOptions loadOptions)
            : sourcePath(std::move(sourcePath)), loadOptions(loadOptions)
        {
        }

        std::filesystem::path sourcePath = {};
        gns::assets::AssetLoadOptions loadOptions = {};
        gns::assets::AssetLoadResult result = {};

        void Execute() override
        {
            GNS_PROFILE_FUNCTION();
            result = gns::assets::AssetLoader::LoadSourceAsset(sourcePath, loadOptions);
        }
    };

    std::string BuildSourceKey(
        const std::filesystem::path& sourcePath,
        const gns::assets::AssetLoadOptions& loadOptions)
    {
        return gns::path::Normalize(sourcePath).generic_string() +
            "|flatten=" + std::to_string(loadOptions.flattenHierarchy) +
            "|skeleton=" + std::to_string(loadOptions.importSkeleton) +
            "|materials=" + std::to_string(loadOptions.importMaterials) +
            "|textures=" + std::to_string(loadOptions.importTextures);
    }

    std::vector<gns::assets::LoadedObject> FilterLoadedObjectsForRequest(
        const std::vector<gns::assets::LoadedObject>& loadedObjects,
        gns::Handle requestedAssetHandle)
    {
        if (!requestedAssetHandle.IsValid())
        {
            return loadedObjects;
        }

        std::vector<gns::assets::LoadedObject> filteredObjects;
        for (const gns::assets::LoadedObject& loadedObject : loadedObjects)
        {
            if (loadedObject.objectHandle == requestedAssetHandle)
            {
                filteredObjects.emplace_back(loadedObject);
            }
        }
        return filteredObjects;
    }

    void AssignLoadedObjectsToRequest(
        gns::assets::AssetRequestEntry& request,
        const std::vector<gns::assets::LoadedObject>& loadedObjects)
    {
        request.loadedObjects = FilterLoadedObjectsForRequest(loadedObjects, request.assetHandle);
        if (request.assetHandle.IsValid() && request.loadedObjects.empty())
        {
            request.state = gns::assets::AssetRequestState::Failed;
            request.error = "Requested asset handle was not produced by the source load.";
            request.loaded = false;
            return;
        }

        request.state = gns::assets::AssetRequestState::Ready;
        request.loaded = true;
    }
}

void gns::assets::AssetSystem::OnUpdate(float deltaTime)
{
    GNS_PROFILE_FUNCTION();
    Flush();
}

gns::assets::AssetRequestHandle gns::assets::AssetSystem::RequestAsset(const std::string& assetPath)
{
    GNS_PROFILE_FUNCTION();
    return RequestAsset(assetPath, AssetLoadOptions{});
}

gns::assets::AssetRequestHandle gns::assets::AssetSystem::RequestAsset(
    const std::string& assetPath,
    const AssetLoadOptions& loadOptions)
{
    GNS_PROFILE_FUNCTION();
    return CreateRequest(gns::path::Normalize(assetPath), loadOptions);
}

gns::assets::AssetRequestHandle gns::assets::AssetSystem::RequestAsset(const Handle handle)
{
    GNS_PROFILE_FUNCTION();

    AssetSourceReference source;
    if (AssetManager::ResolveAssetSource(handle, source) && source.valid)
    {
        return CreateRequest(source.sourcePath, source.loadOptions, handle);
    }

    AssetRequestHandle requestHandle{ .handle = m_nextRequestHandle++ };
    AssetRequestEntry request;
    request.requestHandle = requestHandle;
    request.assetHandle = handle;
    request.state = AssetRequestState::Failed;
    request.error = "Could not resolve asset handle to a loadable source file.";
    assetRequests[requestHandle.handle] = std::move(request);

    LOG_WARNING("[AssetSystem]: Could not resolve async asset request handle.");
    LOG_WARNING(std::to_string(handle.Get()));
    return requestHandle;
}

void gns::assets::AssetSystem::QueueAsset(const Handle handle)
{
    GNS_PROFILE_FUNCTION();
    if (!handle.IsValid() || m_queuedAssetRequests.contains(handle.Get()))
    {
        return;
    }

    const AssetRequestHandle requestHandle = RequestAsset(handle);
    if (requestHandle.IsValid())
    {
        m_queuedAssetRequests[handle.Get()] = requestHandle;
    }
}

gns::assets::AssetRequestState gns::assets::AssetSystem::GetRequestState(
    AssetRequestHandle requestHandle) const
{
    GNS_PROFILE_FUNCTION();
    const auto request = assetRequests.find(requestHandle.handle);
    if (request == assetRequests.end())
    {
        return AssetRequestState::None;
    }

    return request->second.state;
}

bool gns::assets::AssetSystem::IsRequestReady(AssetRequestHandle requestHandle) const
{
    GNS_PROFILE_FUNCTION();
    return GetRequestState(requestHandle) == AssetRequestState::Ready;
}

const std::vector<gns::assets::LoadedObject>* gns::assets::AssetSystem::GetLoadedObjects(
    AssetRequestHandle requestHandle) const
{
    GNS_PROFILE_FUNCTION();
    const auto request = assetRequests.find(requestHandle.handle);
    if (request == assetRequests.end() || request->second.state != AssetRequestState::Ready)
    {
        return nullptr;
    }

    return &request->second.loadedObjects;
}

void gns::assets::AssetSystem::ReleaseRequest(AssetRequestHandle requestHandle)
{
    GNS_PROFILE_FUNCTION();
    auto request = assetRequests.find(requestHandle.handle);
    if (request != assetRequests.end())
    {
        request->second.state = AssetRequestState::Released;
        assetRequests.erase(request);
    }
}

std::vector<gns::assets::LoadedObject> gns::assets::AssetSystem::LoadAsset(const std::string& path)
{
    GNS_PROFILE_FUNCTION();
    return AssetManager::LoadAsset(path);
}

std::vector<gns::assets::LoadedObject> gns::assets::AssetSystem::LoadAsset(
    const std::string& path,
    const AssetLoadOptions& loadOptions)
{
    GNS_PROFILE_FUNCTION();
    return AssetManager::LoadAsset(path, loadOptions);
}

std::vector<gns::assets::LoadedObject> gns::assets::AssetSystem::CommitLoadedAsset(
    const AssetLoadResult& result)
{
    GNS_PROFILE_FUNCTION();
    return AssetManager::CommitLoadedAsset(result);
}

gns::Mesh* gns::assets::AssetSystem::EnsureMeshLoaded(Handle meshHandle)
{
    GNS_PROFILE_FUNCTION();
    return AssetManager::EnsureMeshLoaded(meshHandle);
}

gns::Material* gns::assets::AssetSystem::EnsureMaterialLoaded(Handle materialHandle)
{
    GNS_PROFILE_FUNCTION();
    return AssetManager::EnsureMaterialLoaded(materialHandle);
}

gns::Texture* gns::assets::AssetSystem::EnsureTextureLoaded(Handle textureHandle)
{
    GNS_PROFILE_FUNCTION();
    return AssetManager::EnsureTextureLoaded(textureHandle);
}

bool gns::assets::AssetSystem::ApplyImportedMaterialDefaults(gns::Material& material)
{
    GNS_PROFILE_FUNCTION();
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
    GNS_PROFILE_FUNCTION();
    for (auto batch = m_inFlightSourceLoads.begin(); batch != m_inFlightSourceLoads.end();)
    {
        GNS_PROFILE_SCOPE("AssetSystem::Flush::PollSourceLoad");
        AssetLoadJob* completedJob = jobs::JobSystem::GetCompleted<AssetLoadJob>(batch->second.jobHandle);
        if (completedJob == nullptr)
        {
            ++batch;
            continue;
        }

        CompleteSourceLoad(batch->first, batch->second, completedJob->result);
        jobs::JobSystem::Release(batch->second.jobHandle);
        batch = m_inFlightSourceLoads.erase(batch);
    }

    for (auto request = m_queuedAssetRequests.begin(); request != m_queuedAssetRequests.end();)
    {
        GNS_PROFILE_SCOPE("AssetSystem::Flush::QueuedAssetRequest");
        const AssetRequestState requestState = GetRequestState(request->second);
        if (requestState == AssetRequestState::Ready ||
            requestState == AssetRequestState::Failed ||
            requestState == AssetRequestState::None ||
            requestState == AssetRequestState::Released)
        {
            ReleaseRequest(request->second);
            request = m_queuedAssetRequests.erase(request);
            continue;
        }

        ++request;
    }
}

gns::assets::AssetRequestHandle gns::assets::AssetSystem::CreateRequest(
    const std::filesystem::path& sourcePath,
    const AssetLoadOptions& loadOptions,
    Handle requestedAssetHandle)
{
    GNS_PROFILE_FUNCTION();
    const std::filesystem::path normalizedSourcePath = gns::path::Normalize(sourcePath);
    const std::string sourceKey = BuildSourceKey(normalizedSourcePath, loadOptions);

    AssetRequestHandle requestHandle{ .handle = m_nextRequestHandle++ };
    AssetRequestEntry request;
    request.requestHandle = requestHandle;
    request.path = normalizedSourcePath.string();
    request.assetHandle = requestedAssetHandle;
    request.loadOptions = loadOptions;
    request.state = AssetRequestState::Queued;

    if (const auto loadedSource = m_loadedSources.find(sourceKey); loadedSource != m_loadedSources.end())
    {
        AssignLoadedObjectsToRequest(request, loadedSource->second);
        assetRequests[requestHandle.handle] = std::move(request);
        return requestHandle;
    }

    request.state = AssetRequestState::Loading;
    assetRequests[requestHandle.handle] = std::move(request);

    if (auto inFlight = m_inFlightSourceLoads.find(sourceKey); inFlight != m_inFlightSourceLoads.end())
    {
        inFlight->second.requests.emplace_back(requestHandle);
        return requestHandle;
    }

    AssetLoadJob job(normalizedSourcePath, loadOptions);
    const jobs::JobHandle jobHandle = jobs::JobSystem::CreateJob<AssetLoadJob>(std::move(job));
    jobs::JobSystem::Schedule(jobHandle);

    AssetSourceBatch batch;
    batch.key = sourceKey;
    batch.sourcePath = normalizedSourcePath;
    batch.loadOptions = loadOptions;
    batch.jobHandle = jobHandle;
    batch.requests.emplace_back(requestHandle);
    m_inFlightSourceLoads.emplace(sourceKey, std::move(batch));
    return requestHandle;
}

void gns::assets::AssetSystem::CompleteSourceLoad(
    const std::string& sourceKey,
    AssetSourceBatch& batch,
    const AssetLoadResult& result)
{
    GNS_PROFILE_FUNCTION();
    if (!result.success)
    {
        for (AssetRequestHandle requestHandle : batch.requests)
        {
            auto request = assetRequests.find(requestHandle.handle);
            if (request == assetRequests.end())
            {
                continue;
            }

            request->second.state = AssetRequestState::Failed;
            request->second.error = result.error;
            request->second.loaded = false;
        }
        return;
    }

    std::vector<LoadedObject> loadedObjects;
    {
        GNS_PROFILE_SCOPE("AssetSystem::CompleteSourceLoad::CommitLoadedAsset");
        loadedObjects = CommitLoadedAsset(result);
    }
    m_loadedSources[sourceKey] = loadedObjects;

    for (AssetRequestHandle requestHandle : batch.requests)
    {
        auto request = assetRequests.find(requestHandle.handle);
        if (request == assetRequests.end())
        {
            continue;
        }

        AssignLoadedObjectsToRequest(request->second, loadedObjects);
    }
}
