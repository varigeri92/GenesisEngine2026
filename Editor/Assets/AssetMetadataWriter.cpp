#include "AssetMetadataWriter.h"

#include <string>

#include <yaml-cpp/yaml.h>

#include "../../Engine/Assets/AssetManager.h"
#include "../../Engine/Log/Logger.h"
#include "../../Engine/Utils/Path.h"


uint32_t AssetImporterVersion = 1;

namespace
{
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

    YAML::Emitter emitter;
    emitter << YAML::BeginMap;
    emitter << YAML::Key << "assetType" << YAML::Value << AssetTypeToString(gns::assets::Mesh);
    emitter << YAML::Key << "sourcePath" << YAML::Value << sourcePath;
    emitter << YAML::Key << "importerVersion" << YAML::Value << AssetImporterVersion;
    emitter << YAML::Key << "importOptions" << YAML::Value << YAML::BeginMap;
    emitter << YAML::Key << "flattenHierarchy" << YAML::Value << loadOptions.flattenHierarchy;
    emitter << YAML::Key << "importSkeleton" << YAML::Value << loadOptions.importSkeleton;
    emitter << YAML::Key << "importMaterials" << YAML::Value << loadOptions.importMaterials;
    emitter << YAML::Key << "importTextures" << YAML::Value << loadOptions.importTextures;
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