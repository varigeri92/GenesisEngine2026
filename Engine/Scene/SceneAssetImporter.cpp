#include "gnspch.h"
#include "SceneAssetImporter.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "../Assets/AssetSystem.h"
#include "../Core/ComponentLibrary.h"
#include "../Core/Entity.h"
#include "../Object/Material.h"
#include "../Object/Mesh.h"
#include "../Renderer/RenderSystem.h"
#include "../Renderer/Shader.h"
#include "../Systems/SystemsManager.h"
#include "../Utils/Path.h"
#include "SceneManager.h"

namespace
{
    struct PendingSceneAssetImport
    {
        gns::SceneAssetImportHandle importHandle = {};
        std::filesystem::path assetPath = {};
        gns::assets::AssetLoadOptions loadOptions = {};
        gns::assets::AssetRequestHandle assetRequest = {};
        gns::SceneAssetImportState state = gns::SceneAssetImportState::None;
        std::string error = {};
    };

    size_t NextSceneAssetImportHandle = 1;
    std::unordered_map<size_t, PendingSceneAssetImport> PendingSceneAssetImports;

    bool CreateLoadedMeshObjectsInScene(
        const std::filesystem::path& assetPath,
        const std::vector<gns::assets::LoadedObject>& loaded,
        gns::assets::AssetSystem& assetSystem,
        gns::RenderSystem& renderSystem)
    {
        GNS_PROFILE_FUNCTION();
        if (!renderSystem.EnsureDefaultMeshResources())
        {
            LOG_ERROR("[SceneAssetImporter]: Cannot load mesh asset because default mesh resources are missing.");
            return false;
        }

        gns::Shader* shader = gns::Object::Get<gns::Shader>(renderSystem.GetDefaultMeshShaderHandle());
        gns::Material* defaultMaterial = gns::Object::Get<gns::Material>(renderSystem.GetDefaultMeshMaterialHandle());
        if (shader == nullptr || defaultMaterial == nullptr)
        {
            LOG_ERROR("[SceneAssetImporter]: Cannot load mesh asset because default mesh resources are invalid.");
            return false;
        }

        if (loaded.empty())
        {
            LOG_WARNING("[SceneAssetImporter]: Mesh asset produced no loadable meshes.");
            LOG_WARNING(assetPath.string());
            return false;
        }

        bool createdAny = false;
        gns::Entity rootEntity;
        {
            GNS_PROFILE_SCOPE("SceneAssetImporter::CreateLoadedMeshObjectsInScene::CreateRootEntity");
            rootEntity = gns::Entity::CreateEntity(gns::path::FileStem(assetPath));
        }
        Transform& rootTransform = rootEntity.GetComponent<Transform>();
        rootTransform.position = glm::vec3(0.0f);
        rootTransform.rotation = glm::vec3(0.0f);
        rootTransform.scale = glm::vec3(1.0f);

        const gns::Reference<gns::Material> defaultMaterialRef = defaultMaterial->Ref<gns::Material>();
        const gns::Reference<gns::Shader> shaderRef = shader->Ref<gns::Shader>();
        for (const gns::assets::LoadedObject& loadedObject : loaded)
        {
            GNS_PROFILE_SCOPE("SceneAssetImporter::CreateLoadedMeshObjectsInScene::LoadedObject");
            gns::Mesh* mesh = loadedObject.As<gns::Mesh>();
            if (mesh == nullptr)
            {
                continue;
            }

            {
                GNS_PROFILE_SCOPE("SceneAssetImporter::CreateLoadedMeshObjectsInScene::ApplyMesh");
                renderSystem.ApplyMesh(*mesh);
            }

            gns::Reference<gns::Material> meshMaterial = defaultMaterialRef;
            if (loadedObject.materialHandle.IsValid())
            {
                GNS_PROFILE_SCOPE("SceneAssetImporter::CreateLoadedMeshObjectsInScene::ResolveMaterial");
                gns::Material* loadedMaterial = gns::Object::Get<gns::Material>(loadedObject.materialHandle);
                if (loadedMaterial != nullptr)
                {
                    loadedMaterial->shader_ref = shaderRef;
                    {
                        GNS_PROFILE_SCOPE("SceneAssetImporter::CreateLoadedMeshObjectsInScene::ApplyMaterialInitial");
                        renderSystem.ApplyMaterial(*loadedMaterial);
                    }
                    if (assetSystem.ApplyImportedMaterialDefaults(*loadedMaterial))
                    {
                        GNS_PROFILE_SCOPE("SceneAssetImporter::CreateLoadedMeshObjectsInScene::ApplyMaterialDefaults");
                        renderSystem.ApplyMaterial(*loadedMaterial);
                    }
                    meshMaterial = loadedMaterial->Ref<gns::Material>();
                }
            }

            const std::string name = mesh->GetName();
            {
                GNS_PROFILE_SCOPE("SceneAssetImporter::CreateLoadedMeshObjectsInScene::CreateMeshEntity");
                gns::Entity entity = gns::Entity::CreateEntity(
                    name,
                    gns::SceneManager::GetActiveScene().handle,
                    rootEntity.entity_handle);
                Transform& transform = entity.GetComponent<Transform>();
                transform.position = loadedObject.position;
                transform.rotation = loadedObject.rotation;
                transform.scale = loadedObject.scale;

                MeshComponent& meshComp = entity.AddComponent<MeshComponent>();
                meshComp.mesh = mesh->Ref<gns::Mesh>();
                meshComp.material = meshMaterial;
            }
            LOG_INFO(name);
            createdAny = true;
        }

        if (!createdAny)
        {
            rootEntity.Delete();
        }

        return createdAny;
    }
}

gns::SceneAssetImportHandle gns::SceneAssetImporter::RequestMeshAssetIntoScene(
    const std::filesystem::path& assetPath)
{
    GNS_PROFILE_FUNCTION();
    return RequestMeshAssetIntoScene(assetPath, assets::AssetLoadOptions{});
}

gns::SceneAssetImportHandle gns::SceneAssetImporter::RequestMeshAssetIntoScene(
    const std::filesystem::path& assetPath,
    const assets::AssetLoadOptions& loadOptions)
{
    GNS_PROFILE_FUNCTION();
    assets::AssetSystem* assetSystem = core::SystemsManager::GetSystem<assets::AssetSystem>();
    if (assetSystem == nullptr)
    {
        LOG_ERROR("[SceneAssetImporter]: Cannot request mesh asset because AssetSystem is missing.");
        return {};
    }

    const std::filesystem::path normalizedAssetPath = gns::path::Normalize(assetPath);
    const assets::AssetRequestHandle assetRequest = assetSystem->RequestAsset(
        normalizedAssetPath.string(),
        loadOptions);
    if (!assetRequest.IsValid())
    {
        LOG_ERROR("[SceneAssetImporter]: AssetSystem rejected mesh asset request.");
        LOG_ERROR(normalizedAssetPath.string());
        return {};
    }

    SceneAssetImportHandle importHandle{ .handle = NextSceneAssetImportHandle++ };
    PendingSceneAssetImport pendingImport;
    pendingImport.importHandle = importHandle;
    pendingImport.assetPath = normalizedAssetPath;
    pendingImport.loadOptions = loadOptions;
    pendingImport.assetRequest = assetRequest;
    pendingImport.state = SceneAssetImportState::Loading;
    PendingSceneAssetImports[importHandle.handle] = std::move(pendingImport);
    return importHandle;
}

gns::SceneAssetImportState gns::SceneAssetImporter::GetImportState(SceneAssetImportHandle importHandle)
{
    GNS_PROFILE_FUNCTION();
    const auto import = PendingSceneAssetImports.find(importHandle.handle);
    if (import == PendingSceneAssetImports.end())
    {
        return SceneAssetImportState::None;
    }

    return import->second.state;
}

void gns::SceneAssetImporter::ReleaseImport(SceneAssetImportHandle importHandle)
{
    GNS_PROFILE_FUNCTION();
    auto import = PendingSceneAssetImports.find(importHandle.handle);
    if (import == PendingSceneAssetImports.end())
    {
        return;
    }

    if (assets::AssetSystem* assetSystem = core::SystemsManager::GetSystem<assets::AssetSystem>())
    {
        assetSystem->ReleaseRequest(import->second.assetRequest);
    }
    PendingSceneAssetImports.erase(import);
}

void gns::SceneAssetImporter::UpdatePendingImports()
{
    GNS_PROFILE_FUNCTION();
    assets::AssetSystem* assetSystem = core::SystemsManager::GetSystem<assets::AssetSystem>();
    RenderSystem* renderSystem = core::SystemsManager::GetSystem<RenderSystem>();
    if (assetSystem == nullptr || renderSystem == nullptr)
    {
        return;
    }

    for (auto& [handle, import] : PendingSceneAssetImports)
    {
        if (import.state != SceneAssetImportState::Loading)
        {
            continue;
        }

        const assets::AssetRequestState requestState = assetSystem->GetRequestState(import.assetRequest);
        if (requestState == assets::AssetRequestState::Failed)
        {
            import.state = SceneAssetImportState::Failed;
            assetSystem->ReleaseRequest(import.assetRequest);
            continue;
        }

        if (requestState != assets::AssetRequestState::Ready)
        {
            continue;
        }

        const std::vector<assets::LoadedObject>* loadedObjects = assetSystem->GetLoadedObjects(import.assetRequest);
        if (loadedObjects == nullptr)
        {
            import.state = SceneAssetImportState::Failed;
            assetSystem->ReleaseRequest(import.assetRequest);
            continue;
        }

        import.state = CreateLoadedMeshObjectsInScene(
            import.assetPath,
            *loadedObjects,
            *assetSystem,
            *renderSystem)
            ? SceneAssetImportState::Completed
            : SceneAssetImportState::Failed;
        assetSystem->ReleaseRequest(import.assetRequest);
    }
}

bool gns::SceneAssetImporter::LoadMeshAssetIntoScene(
    const std::filesystem::path& assetPath,
    RenderSystem& renderSystem)
{
    GNS_PROFILE_FUNCTION();
    return LoadMeshAssetIntoScene(assetPath, assets::AssetLoadOptions{}, renderSystem);
}

bool gns::SceneAssetImporter::LoadMeshAssetIntoScene(
    const std::filesystem::path& assetPath,
    const assets::AssetLoadOptions& loadOptions,
    RenderSystem& renderSystem)
{
    GNS_PROFILE_FUNCTION();
    const std::filesystem::path normalizedAssetPath = gns::path::Normalize(assetPath);
    assets::AssetSystem* assetSystem = core::SystemsManager::GetSystem<assets::AssetSystem>();
    if (assetSystem == nullptr)
    {
        LOG_ERROR("[SceneAssetImporter]: Cannot load mesh asset because AssetSystem is missing.");
        return false;
    }
    std::vector<assets::LoadedObject> loaded;
    {
        GNS_PROFILE_SCOPE("SceneAssetImporter::LoadMeshAssetIntoScene::LoadAsset");
        loaded = assetSystem->LoadAsset(normalizedAssetPath.string(), loadOptions);
    }
    return CreateLoadedMeshObjectsInScene(normalizedAssetPath, loaded, *assetSystem, renderSystem);
}
