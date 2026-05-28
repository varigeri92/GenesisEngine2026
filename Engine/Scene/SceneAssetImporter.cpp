#include "gnspch.h"
#include "SceneAssetImporter.h"

#include <string>
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
    if (!renderSystem.EnsureDefaultMeshResources())
    {
        LOG_ERROR("[SceneAssetImporter]: Cannot load mesh asset because default mesh resources are missing.");
        return false;
    }

    Shader* shader = Object::Get<Shader>(renderSystem.GetDefaultMeshShaderHandle());
    Material* defaultMaterial = Object::Get<Material>(renderSystem.GetDefaultMeshMaterialHandle());
    if (shader == nullptr || defaultMaterial == nullptr)
    {
        LOG_ERROR("[SceneAssetImporter]: Cannot load mesh asset because default mesh resources are invalid.");
        return false;
    }

    const std::filesystem::path normalizedAssetPath = gns::path::Normalize(assetPath);
    assets::AssetSystem* assetSystem = core::SystemsManager::GetSystem<assets::AssetSystem>();
    if (assetSystem == nullptr)
    {
        LOG_ERROR("[SceneAssetImporter]: Cannot load mesh asset because AssetSystem is missing.");
        return false;
    }
    std::vector<gns::assets::LoadedObject> loaded;
    {
        GNS_PROFILE_SCOPE("SceneAssetImporter::LoadMeshAssetIntoScene::LoadAsset");
        loaded = assetSystem->LoadAsset(normalizedAssetPath.string(), loadOptions);
    }
    if (loaded.empty())
    {
        LOG_WARNING("[SceneAssetImporter]: Mesh asset produced no loadable meshes.");
        LOG_WARNING(normalizedAssetPath.string());
        return false;
    }

    bool createdAny = false;
    Entity rootEntity;
    {
        GNS_PROFILE_SCOPE("SceneAssetImporter::LoadMeshAssetIntoScene::CreateRootEntity");
        rootEntity = Entity::CreateEntity(gns::path::FileStem(normalizedAssetPath));
    }
    Transform& rootTransform = rootEntity.GetComponent<Transform>();
    rootTransform.position = glm::vec3(0.0f);
    rootTransform.rotation = glm::vec3(0.0f);
    rootTransform.scale = glm::vec3(1.0f);

    const Reference<Material> defaultMaterialRef = defaultMaterial->Ref<Material>();
    const Reference<Shader> shaderRef = shader->Ref<Shader>();
    for (auto& loadedObject : loaded)
    {
        GNS_PROFILE_SCOPE("SceneAssetImporter::LoadMeshAssetIntoScene::LoadedObject");
        Mesh* mesh = loadedObject.As<Mesh>();
        if (mesh == nullptr)
        {
            continue;
        }

        {
            GNS_PROFILE_SCOPE("SceneAssetImporter::LoadMeshAssetIntoScene::ApplyMesh");
            renderSystem.ApplyMesh(*mesh);
        }

        Reference<Material> meshMaterial = defaultMaterialRef;
        if (loadedObject.materialHandle.IsValid())
        {
            GNS_PROFILE_SCOPE("SceneAssetImporter::LoadMeshAssetIntoScene::ResolveMaterial");
            Material* loadedMaterial = Object::Get<Material>(loadedObject.materialHandle);
            if (loadedMaterial != nullptr)
            {
                loadedMaterial->shader_ref = shaderRef;
                {
                    GNS_PROFILE_SCOPE("SceneAssetImporter::LoadMeshAssetIntoScene::ApplyMaterialInitial");
                    renderSystem.ApplyMaterial(*loadedMaterial);
                }
                if (assetSystem->ApplyImportedMaterialDefaults(*loadedMaterial))
                {
                    GNS_PROFILE_SCOPE("SceneAssetImporter::LoadMeshAssetIntoScene::ApplyMaterialDefaults");
                    renderSystem.ApplyMaterial(*loadedMaterial);
                }
                meshMaterial = loadedMaterial->Ref<Material>();
            }
        }

        const std::string name = mesh->GetName();
        {
            GNS_PROFILE_SCOPE("SceneAssetImporter::LoadMeshAssetIntoScene::CreateMeshEntity");
            Entity entity = Entity::CreateEntity(name, SceneManager::GetActiveScene().handle, rootEntity.entity_handle);
            Transform& transform = entity.GetComponent<Transform>();
            transform.position = loadedObject.position;
            transform.rotation = loadedObject.rotation;
            transform.scale = loadedObject.scale;

            MeshComponent& meshComp = entity.AddComponent<MeshComponent>();
            meshComp.mesh = mesh->Ref<Mesh>();
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
