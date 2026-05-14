#include "AssetMetadataWriter.h"

#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <yaml-cpp/yaml.h>

#include "../../Engine/Assets/AssetManager.h"
#include "../../Engine/Log/Logger.h"
#include "../../Engine/Utils/Path.h"


uint32_t AssetImporterVersion = 1;

namespace
{
    struct TextureArtifact
    {
        gns::Handle handle;
        std::string path;
    };

    struct MaterialArtifact
    {
        gns::Handle handle;
        uint32_t materialIndex = 0;
        std::string name;
        std::string path;
    };

    std::string AssetTypeToString(gns::assets::AssetType assetType)
    {
        switch (assetType)
        {
        case gns::assets::Mesh:
            return "Mesh";
        case gns::assets::Texture:
            return "Texture";
        case gns::assets::Shader:
            return "Shader";
        case gns::assets::ComputeShader:
            return "ComputeShader";
        case gns::assets::Material:
            return "Material";
        default:
            return "Generic";
        }
    }

    std::string ToProjectRelativeString(const std::filesystem::path& path)
    {
        const std::filesystem::path normalizedPath = gns::path::Normalize(path);
        const std::filesystem::path projectRoot = gns::path::ProjectDirectory();
        if (projectRoot.empty())
        {
            return normalizedPath.generic_string();
        }

        return gns::path::ToRelative(normalizedPath, projectRoot).generic_string();
    }

    std::filesystem::path ResolveTexturePath(
        const std::filesystem::path& assetDirectory,
        const aiString& texturePath)
    {
        return gns::path::ResolveAgainst(assetDirectory, std::filesystem::path(texturePath.C_Str()));
    }

    std::string MaterialName(const aiMaterial* material, uint32_t materialIndex)
    {
        if (material != nullptr && material->GetName().length > 0)
        {
            return material->GetName().C_Str();
        }

        return "Material_" + std::to_string(materialIndex);
    }

    bool TryGetMaterialTexturePath(
        const aiMaterial* material,
        aiTextureType textureType,
        aiString& texturePath)
    {
        return material != nullptr &&
            material->GetTextureCount(textureType) > 0 &&
            material->GetTexture(textureType, 0, &texturePath) == AI_SUCCESS &&
            texturePath.length > 0;
    }

    gns::Handle CollectAlbedoTexture(
        const aiMaterial* material,
        const std::filesystem::path& assetDirectory,
        const std::string& sourcePath,
        std::vector<TextureArtifact>& textureArtifacts)
    {
        aiString texturePath;
        if (!TryGetMaterialTexturePath(material, aiTextureType_BASE_COLOR, texturePath) &&
            !TryGetMaterialTexturePath(material, aiTextureType_DIFFUSE, texturePath))
        {
            return {};
        }

        std::string textureArtifactPath;
        if (texturePath.C_Str()[0] == '*')
        {
            textureArtifactPath = sourcePath + "::embedded_texture_" + std::string(texturePath.C_Str() + 1);
        }
        else
        {
            textureArtifactPath = ToProjectRelativeString(ResolveTexturePath(assetDirectory, texturePath));
        }

        const gns::Handle textureHandle = gns::assets::AssetManager::GetTextureArtifactHandle(textureArtifactPath);
        const auto exists = std::find_if(
            textureArtifacts.begin(),
            textureArtifacts.end(),
            [&](const TextureArtifact& artifact)
            {
                return artifact.handle == textureHandle;
            });

        if (exists == textureArtifacts.end())
        {
            textureArtifacts.push_back(TextureArtifact
            {
                .handle = textureHandle,
                .path = textureArtifactPath
            });
        }

        return textureHandle;
    }

    std::filesystem::path MaterialFilePath(
        const std::filesystem::path& modelPath,
        uint32_t materialIndex)
    {
        const std::filesystem::path materialDirectory = gns::path::ParentDirectory(modelPath) / "Materials";
        return materialDirectory /
            (gns::path::FileStem(modelPath) + "_material_" + std::to_string(materialIndex) + ".gnsmaterial");
    }

    bool WriteMaterialFile(const std::filesystem::path& materialPath, const MaterialArtifact& material)
    {
        std::error_code error;
        std::filesystem::create_directories(materialPath.parent_path(), error);
        if (error)
        {
            LOG_WARNING("[AssetMetadataWriter]: Failed to create material directory.");
            LOG_WARNING(materialPath.parent_path().string());
            return false;
        }

        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "assetType" << YAML::Value << "Material";
        emitter << YAML::Key << "handle" << YAML::Value << material.handle.Get();
        emitter << YAML::Key << "name" << YAML::Value << material.name;
        emitter << YAML::EndMap;

        return emitter.good() && gns::path::WriteTextFile(materialPath, emitter.c_str());
    }

    std::filesystem::path ArtifactDirectory()
    {
        return gns::path::Resolve(gns::path::Root::ProjectLibrary, "Artifacts");
    }

    bool WriteArtifactLink(gns::Handle handle, const std::filesystem::path& metaPath)
    {
        if (!handle.IsValid())
        {
            return true;
        }

        const std::filesystem::path artifactDirectory = ArtifactDirectory();
        std::error_code error;
        std::filesystem::create_directories(artifactDirectory, error);
        if (error)
        {
            LOG_WARNING("[AssetMetadataWriter]: Failed to create artifact directory.");
            LOG_WARNING(artifactDirectory.string());
            return false;
        }

        const std::filesystem::path linkPath = artifactDirectory / std::to_string(handle.Get());
        if (!gns::path::WriteTextFile(linkPath, ToProjectRelativeString(metaPath)))
        {
            LOG_WARNING("[AssetMetadataWriter]: Failed to write artifact link.");
            LOG_WARNING(linkPath.string());
            return false;
        }

        return true;
    }

    void WriteArtifactLinks(
        const std::filesystem::path& metaPath,
        gns::Handle sourceHandle,
        const std::vector<gns::Handle>& meshHandles,
        const std::vector<MaterialArtifact>& materialArtifacts,
        const std::vector<TextureArtifact>& textureArtifacts)
    {
        WriteArtifactLink(sourceHandle, metaPath);

        for (gns::Handle meshHandle : meshHandles)
        {
            WriteArtifactLink(meshHandle, metaPath);
        }

        for (const MaterialArtifact& material : materialArtifacts)
        {
            WriteArtifactLink(material.handle, metaPath);
        }

        for (const TextureArtifact& texture : textureArtifacts)
        {
            WriteArtifactLink(texture.handle, metaPath);
        }
    }
}

bool editor::assets::WriteModelMetaFile(
    const std::filesystem::path& modelPath,
    const gns::assets::AssetLoadOptions& loadOptions)
{
    const std::filesystem::path normalizedModelPath = gns::path::Normalize(modelPath);
    std::filesystem::path metaPath = normalizedModelPath;
    metaPath += ".meta";

    const std::filesystem::path projectRoot = gns::path::ProjectDirectory();
    const std::string sourcePath = projectRoot.empty()
        ? normalizedModelPath.generic_string()
        : gns::path::ToRelative(normalizedModelPath, projectRoot).generic_string();

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        normalizedModelPath.string(),
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType);
    if (scene == nullptr)
    {
        LOG_ERROR("[AssetMetadataWriter]: Failed to inspect model for metadata artifacts.");
        LOG_ERROR(importer.GetErrorString());
        return false;
    }

    std::vector<TextureArtifact> textureArtifacts;
    std::vector<MaterialArtifact> materialArtifacts;
    std::vector<gns::Handle> meshArtifactHandles;
    if (scene->HasMaterials())
    {
        materialArtifacts.reserve(scene->mNumMaterials);
        const std::filesystem::path assetDirectory = gns::path::ParentDirectory(normalizedModelPath);
        for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
        {
            const aiMaterial* assimpMaterial = scene->mMaterials[materialIndex];
            const std::filesystem::path materialPath = MaterialFilePath(normalizedModelPath, materialIndex);
            MaterialArtifact material
            {
                .handle = gns::assets::AssetManager::GetMaterialArtifactHandle(sourcePath, materialIndex),
                .materialIndex = materialIndex,
                .name = MaterialName(assimpMaterial, materialIndex),
                .path = ToProjectRelativeString(materialPath)
            };

            if (loadOptions.importTextures)
            {
                (void)CollectAlbedoTexture(assimpMaterial, assetDirectory, sourcePath, textureArtifacts);
            }

            if (loadOptions.importMaterials && !WriteMaterialFile(materialPath, material))
            {
                LOG_WARNING("[AssetMetadataWriter]: Failed to write material asset file.");
                LOG_WARNING(materialPath.string());
            }

            materialArtifacts.push_back(material);
        }
    }

    YAML::Emitter emitter;
    emitter << YAML::BeginMap;
    emitter << YAML::Key << "assetType" << YAML::Value << AssetTypeToString(gns::assets::Mesh);
    emitter << YAML::Key << "assetHandle" << YAML::Value
        << gns::assets::AssetManager::GetModelAssetHandle(sourcePath).Get();
    emitter << YAML::Key << "sourcePath" << YAML::Value << sourcePath;
    emitter << YAML::Key << "importerVersion" << YAML::Value << AssetImporterVersion;
    emitter << YAML::Key << "importOptions" << YAML::Value << YAML::BeginMap;
    emitter << YAML::Key << "flattenHierarchy" << YAML::Value << loadOptions.flattenHierarchy;
    emitter << YAML::Key << "importSkeleton" << YAML::Value << loadOptions.importSkeleton;
    emitter << YAML::Key << "importMaterials" << YAML::Value << loadOptions.importMaterials;
    emitter << YAML::Key << "importTextures" << YAML::Value << loadOptions.importTextures;
    emitter << YAML::EndMap;
    emitter << YAML::Key << "artifacts" << YAML::Value << YAML::BeginMap;

    emitter << YAML::Key << "meshes" << YAML::Value << YAML::BeginSeq;
    if (scene->HasMeshes())
    {
        meshArtifactHandles.reserve(scene->mNumMeshes);
    }

    for (uint32_t meshIndex = 0; scene->HasMeshes() && meshIndex < scene->mNumMeshes; ++meshIndex)
    {
        const aiMesh* mesh = scene->mMeshes[meshIndex];
        std::string meshName = mesh != nullptr ? mesh->mName.C_Str() : "";
        if (meshName.empty())
        {
            meshName = "Mesh_" + std::to_string(meshIndex);
        }

        const gns::Handle meshHandle = gns::assets::AssetManager::GetMeshArtifactHandle(sourcePath, meshIndex);
        meshArtifactHandles.push_back(meshHandle);

        gns::Handle materialHandle;
        if (mesh != nullptr && mesh->mMaterialIndex < materialArtifacts.size())
        {
            materialHandle = materialArtifacts[mesh->mMaterialIndex].handle;
        }

        emitter << YAML::BeginMap;
        emitter << YAML::Key << "handle" << YAML::Value << meshHandle.Get();
        emitter << YAML::Key << "meshIndex" << YAML::Value << meshIndex;
        emitter << YAML::Key << "name" << YAML::Value << meshName;
        if (materialHandle.IsValid())
        {
            emitter << YAML::Key << "material" << YAML::Value << materialHandle.Get();
        }
        emitter << YAML::EndMap;
    }
    emitter << YAML::EndSeq;

    emitter << YAML::Key << "materials" << YAML::Value << YAML::BeginSeq;
    for (const MaterialArtifact& material : materialArtifacts)
    {
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "handle" << YAML::Value << material.handle.Get();
        emitter << YAML::Key << "materialIndex" << YAML::Value << material.materialIndex;
        emitter << YAML::Key << "name" << YAML::Value << material.name;
        emitter << YAML::Key << "path" << YAML::Value << material.path;
        emitter << YAML::EndMap;
    }
    emitter << YAML::EndSeq;

    emitter << YAML::Key << "textures" << YAML::Value << YAML::BeginSeq;
    for (const TextureArtifact& texture : textureArtifacts)
    {
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "handle" << YAML::Value << texture.handle.Get();
        emitter << YAML::Key << "path" << YAML::Value << texture.path;
        emitter << YAML::EndMap;
    }
    emitter << YAML::EndSeq;
    emitter << YAML::EndMap;
    emitter << YAML::EndMap;

    if (!emitter.good())
    {
        LOG_ERROR("[AssetMetadataWriter]: Failed to create model meta YAML.");
        return false;
    }

    if (!gns::path::WriteTextFile(metaPath, emitter.c_str()))
    {
        LOG_ERROR("[AssetMetadataWriter]: Failed to write model meta file.");
        LOG_ERROR(metaPath.string());
        return false;
    }

    LOG_INFO("[AssetMetadataWriter]: Wrote model meta file.");
    LOG_INFO(metaPath.string());
    WriteArtifactLinks(
        metaPath,
        gns::assets::AssetManager::GetModelAssetHandle(sourcePath),
        meshArtifactHandles,
        materialArtifacts,
        textureArtifacts);

    return true;
}


void editor::assets::ReadMetadataFromFile(std::filesystem::path metaPath, gns::assets::AssetLoadOptions& options)
{
    YAML::Node root = YAML::LoadFile(metaPath.string());
    
    uint32_t version = root["importerVersion"].as<uint32_t>();
    if (version != AssetImporterVersion)
    {
        LOG_WARNING("[AssetMetadataWriter]: Importer version mismatch! \n \t Asset may load incorrectly!");
    }
    std::filesystem::path sourcePath = root["sourcePath"].as<std::string>();
    YAML::Node importOptions = root["importOptions"];
    
    options.flattenHierarchy = importOptions["flattenHierarchy"].as<bool>();
    options.importSkeleton = importOptions["importSkeleton"].as<bool>();
    options.importMaterials = importOptions["importMaterials"].as<bool>();
    options.importTextures = importOptions["importTextures"].as<bool>();
}
