#include "gnspch.h"
#include "AssetManager.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "../Object/Mesh.h"

/*
void foo()
{
    std::string assetDir = gns::fileUtils::GetContainingDirectory(mesh_asset.src_path);
    if (assetDir == "")
        assetDir = PathHelper::AssetsPath;
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(PathHelper::FromAssetsRelative(mesh_asset.src_path),
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType);

    if (nullptr == scene) {
        LOG_ERROR(importer.GetErrorString());
        return false;
    }

    if (!scene->HasMeshes())
        return false;

    std::vector<guid> loaded_materialGuids = {};
    loaded_materialGuids.reserve(scene->mNumMeshes);
    gns::RenderSystem* renderSystem = SystemsManager::GetSystem<gns::RenderSystem>();

    std::vector<rendering::Material*> materials = {};

	if (scene->HasMaterials())
    {
        const std::string v_shader_path = R"(Shaders\colored_triangle_mesh.vert)";
        const std::string f_shader_path = R"(Shaders\tex_image.frag)";
        rendering::Shader* shader = renderSystem->CreateShader(
            "default_shader", v_shader_path, f_shader_path);

        for (size_t m = 0; m < scene->mNumMaterials; m++)
        {
        	aiMaterial* mat = scene->mMaterials[m];

            rendering::Material* material = renderSystem->CreateMaterial(shader, mat->GetName().C_Str());
            material->uniformData.metallic_roughness_AO = { 0,1,1,0 };
            renderSystem->ResetMaterialTextures(material);
            materials.push_back(material);

            LoadTextures(renderSystem, material, mat, aiTextureType_NORMALS, assetDir);
            LoadTextures(renderSystem, material, mat, aiTextureType_BASE_COLOR, assetDir);
            LoadTextures(renderSystem, material, mat, aiTextureType_EMISSIVE, assetDir);
            LoadTextures(renderSystem, material, mat, aiTextureType_GLTF_METALLIC_ROUGHNESS, assetDir);
            LoadTextures(renderSystem, material, mat, aiTextureType_AMBIENT_OCCLUSION, assetDir);
        }
    }

	AssetManager::AssetLoadedEvent assetloaded = {};
    assetloaded.assetName = assetInfo.name;
    assetloaded.assetType = AssetType::Mesh;
    assetloaded.loadedAsset = assetInfo.assetGuid;
    assetloaded.primaryObjects.reserve(scene->mNumMeshes);
    assetloaded.secondaryObjects.reserve(scene->mNumMeshes);
    

    for (size_t m = 0; m < scene->mNumMeshes; m++)
    {
        
        
    	if (!mesh_asset.isStatic)
        {
            AssetManager::AssetLoadedEvent dynamic_assetloaded;
            dynamic_assetloaded.assetName = mesh->mName.C_Str();
            dynamic_assetloaded.assetType = AssetType::Mesh;
            dynamic_assetloaded.loadedAsset = mesh_asset.sub_meshes[m].mesh_guid;
            dynamic_assetloaded.primaryObjects.reserve(1);
            dynamic_assetloaded.secondaryObjects.reserve(1);
            dynamic_assetloaded.primaryObjects.push_back(mesh_asset.sub_meshes[m].mesh_guid);
            dynamic_assetloaded.secondaryObjects.push_back(materials[mesh->mMaterialIndex]->getGuid());

            AssetManager::AssetLoadedEventQueue.emplace(dynamic_assetloaded);
        }
    }
    if (mesh_asset.isStatic)
		AssetManager::AssetLoadedEventQueue.emplace(assetloaded);
}
*/

//D:\__ProjectGenesis\GenesisEngine_TestProject\Assets\basicmesh.glb
std::unordered_map<gns::Handle, gns::assets::Asset> gns::assets::AssetManager::AssetMap = {};
std::vector<gns::assets::LoadedObject> gns::assets::AssetManager::LoadAsset(const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile( path,
      aiProcess_CalcTangentSpace       |
      aiProcess_Triangulate            |
      aiProcess_JoinIdenticalVertices  |
      aiProcess_SortByPType);
    if (nullptr == scene) {
        LOG_ERROR( importer.GetErrorString());
        return {};
    }
    if (scene->HasMeshes())
    {
        std::vector<gns::assets::LoadedObject> loaded;
        for (uint32_t m = 0; m < scene->mNumMeshes; m++)
        {
            LOG_INFO(scene->mMeshes[m]->mName.C_Str());
            const aiMesh* mesh = scene->mMeshes[m];
            gns::Mesh* newMesh = gns::Object::Create<gns::Mesh>(mesh->mName.C_Str());

            if (scene->HasMaterials())
            {
                LOG_INFO("Create material");
            }
                
            for (size_t v = 0; v < scene->mMeshes[m]->mNumVertices; v++)
            {
                newMesh->positions.emplace_back(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);
                newMesh->normals.emplace_back(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
                newMesh->colors.emplace_back(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z, 1.f);
                newMesh->uvs.emplace_back(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y * -1);
            }
            if (mesh->HasTangentsAndBitangents())
            {
                for (size_t v = 0; v < scene->mMeshes[m]->mNumVertices; v++)
                {
                    newMesh->tangents.emplace_back(mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z);
                    newMesh->bitangents.emplace_back(mesh->mBitangents[v].x, mesh->mBitangents[v].y, mesh->mBitangents[v].z);
                }
            }
            const size_t startindex = newMesh->indices.size();
            for (uint32_t f = 0; f < mesh->mNumFaces; f++) {
                const aiFace& Face = mesh->mFaces[f];
                for (uint32_t i = 0; i < Face.mNumIndices; i++)
                {
                    uint32_t vi = Face.mIndices[i];
                    newMesh->indices.push_back(vi);
                }
            }
            const uint32_t count = static_cast<uint32_t>(newMesh->indices.size());
            newMesh->bufferRange = {
                .startIndex = static_cast<uint32_t>(startindex),
                .count = count
            };
            loaded.emplace_back(newMesh->GetHandle(), newMesh);
        }
        return loaded;
    }
    LOG_INFO("File: " + path + " does not contain meshes.");
    return {};
}
