#include "AssetMetadataWriter.h"

#include <array>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/GltfMaterial.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <yaml-cpp/yaml.h>

#include <glm/glm.hpp>

#include "../../Engine/Assets/AssetManager.h"
#include "../../Engine/Log/Logger.h"
#include "../../Engine/Object/Material.h"
#include "../../Engine/Renderer/Vulkan/ShaderUtils.h"
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
        glm::vec4 albedoColor = glm::vec4(1.0f);
        glm::vec4 emissiveColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec4 textureTilingOffset = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);
        float metallic = 0.0f;
        float roughness = 1.0f;
        float ambientOcclusion = 1.0f;
        float alpha = 1.0f;
        float normalStrength = 1.0f;
        float emissiveStrength = 0.0f;
        float alphaCutoff = 0.5f;
        gns::Handle albedoTexture;
        gns::Handle normalMap;
        gns::Handle metallicMap;
        gns::Handle roughnessMap;
        gns::Handle ambientOcclusionMap;
        gns::Handle emissiveMap;
    };

    struct MaterialTextureSlot
    {
        aiTextureType type = aiTextureType_NONE;
        uint32_t index = 0;
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

    std::optional<MaterialTextureSlot> FindFirstMaterialTextureSlot(const aiMaterial* material)
    {
        if (material == nullptr)
        {
            return std::nullopt;
        }

        constexpr std::array<aiTextureType, 2> preferredTextureTypes =
        {
            aiTextureType_BASE_COLOR,
            aiTextureType_DIFFUSE
        };

        for (const aiTextureType textureType : preferredTextureTypes)
        {
            if (material->GetTextureCount(textureType) > 0)
            {
                return MaterialTextureSlot{ textureType, 0 };
            }
        }

        for (uint32_t propertyIndex = 0; propertyIndex < material->mNumProperties; ++propertyIndex)
        {
            const aiMaterialProperty* property = material->mProperties[propertyIndex];
            if (property == nullptr || std::strcmp(property->mKey.C_Str(), "$tex.file") != 0)
            {
                continue;
            }

            const aiTextureType textureType = static_cast<aiTextureType>(property->mSemantic);
            if (textureType == aiTextureType_NONE)
            {
                continue;
            }

            return MaterialTextureSlot{ textureType, property->mIndex };
        }

        return std::nullopt;
    }

    std::optional<MaterialTextureSlot> FindMaterialTextureSlot(
        const aiMaterial* material,
        aiTextureType textureType,
        uint32_t textureIndex = 0)
    {
        if (material == nullptr || material->GetTextureCount(textureType) <= textureIndex)
        {
            return std::nullopt;
        }

        return MaterialTextureSlot{ textureType, textureIndex };
    }

    gns::Handle CollectTexture(
        const aiMaterial* material,
        const MaterialTextureSlot& textureSlot,
        const std::filesystem::path& assetDirectory,
        const std::string& sourcePath,
        std::vector<TextureArtifact>& textureArtifacts)
    {
        aiString texturePath;
        if (material == nullptr ||
            material->GetTexture(textureSlot.type, textureSlot.index, &texturePath) != AI_SUCCESS ||
            texturePath.length == 0)
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

    gns::Handle CollectOptionalTexture(
        const aiMaterial* material,
        std::optional<MaterialTextureSlot> textureSlot,
        const std::filesystem::path& assetDirectory,
        const std::string& sourcePath,
        std::vector<TextureArtifact>& textureArtifacts)
    {
        return textureSlot
            ? CollectTexture(material, *textureSlot, assetDirectory, sourcePath, textureArtifacts)
            : gns::Handle{};
    }

    std::filesystem::path MaterialFilePath(
        const std::filesystem::path& modelPath,
        uint32_t materialIndex)
    {
        const std::filesystem::path materialDirectory = gns::path::ParentDirectory(modelPath) / "Materials";
        return materialDirectory /
            (gns::path::FileStem(modelPath) + "_material_" + std::to_string(materialIndex) + ".gnsmaterial");
    }

    void WriteVec4(YAML::Emitter& emitter, const glm::vec4& value)
    {
        emitter << YAML::Flow << YAML::BeginSeq
            << value.x
            << value.y
            << value.z
            << value.w
            << YAML::EndSeq;
    }

    void WriteVec3(YAML::Emitter& emitter, const glm::vec3& value)
    {
        emitter << YAML::Flow << YAML::BeginSeq
            << value.x
            << value.y
            << value.z
            << YAML::EndSeq;
    }

    void WriteVec2(YAML::Emitter& emitter, const glm::vec2& value)
    {
        emitter << YAML::Flow << YAML::BeginSeq
            << value.x
            << value.y
            << YAML::EndSeq;
    }

    bool IsSerializableMaterialProperty(const gns::MaterialPropertyInfo& property)
    {
        if (!property.IsBufferBacked() ||
            property.name.rfind("padding", 0) == 0 ||
            property.name.find(".padding") != std::string::npos)
        {
            return false;
        }

        switch (property.type)
        {
        case gns::MaterialPropertyType::Float:
        case gns::MaterialPropertyType::Int:
        case gns::MaterialPropertyType::UInt:
        case gns::MaterialPropertyType::Vec2:
        case gns::MaterialPropertyType::Vec3:
        case gns::MaterialPropertyType::Vec4:
        case gns::MaterialPropertyType::Color3:
        case gns::MaterialPropertyType::Color4:
            return property.elementCount == 1;
        default:
            return false;
        }
    }

    std::string MaterialPropertyBaseName(const std::string& propertyName)
    {
        const size_t separator = propertyName.find_last_of('.');
        return separator == std::string::npos ? propertyName : propertyName.substr(separator + 1);
    }

    float MaterialFloatValue(const MaterialArtifact& material, const std::string& propertyName)
    {
        const std::string baseName = MaterialPropertyBaseName(propertyName);
        if (baseName == "metallic")
        {
            return material.metallic;
        }
        if (baseName == "roughness")
        {
            return material.roughness;
        }
        if (baseName == "ambient_occlusion")
        {
            return material.ambientOcclusion;
        }
        if (baseName == "alpha")
        {
            return material.alpha;
        }
        if (baseName == "normal_strength")
        {
            return material.normalStrength;
        }
        if (baseName == "emissive_strength")
        {
            return material.emissiveStrength;
        }
        if (baseName == "alpha_cutoff")
        {
            return material.alphaCutoff;
        }

        return 0.0f;
    }

    glm::vec4 MaterialVec4Value(const MaterialArtifact& material, const std::string& propertyName)
    {
        const std::string baseName = MaterialPropertyBaseName(propertyName);
        if (baseName == "albedo_color")
        {
            return material.albedoColor;
        }
        if (baseName == "emissive_color")
        {
            return material.emissiveColor;
        }
        if (baseName == "texture_tiling_offset")
        {
            return material.textureTilingOffset;
        }

        return glm::vec4(0.0f);
    }

    bool WriteMaterialProperty(
        YAML::Emitter& emitter,
        const MaterialArtifact& material,
        const gns::MaterialPropertyInfo& property)
    {
        if (!IsSerializableMaterialProperty(property))
        {
            return false;
        }

        emitter << YAML::Key << property.name << YAML::Value;
        switch (property.type)
        {
        case gns::MaterialPropertyType::Float:
            emitter << MaterialFloatValue(material, property.name);
            return true;
        case gns::MaterialPropertyType::Int:
            emitter << 0;
            return true;
        case gns::MaterialPropertyType::UInt:
            emitter << 0u;
            return true;
        case gns::MaterialPropertyType::Vec2:
            WriteVec2(emitter, glm::vec2(0.0f));
            return true;
        case gns::MaterialPropertyType::Vec3:
        case gns::MaterialPropertyType::Color3:
            WriteVec3(emitter, glm::vec3(MaterialVec4Value(material, property.name)));
            return true;
        case gns::MaterialPropertyType::Vec4:
        case gns::MaterialPropertyType::Color4:
            WriteVec4(emitter, MaterialVec4Value(material, property.name));
            return true;
        default:
            return false;
        }
    }

    gns::MaterialLayout BuildFallbackMaterialLayout()
    {
        gns::MaterialLayout layout;
        layout.AddProperty("albedo_color", gns::MaterialPropertyType::Color4);
        layout.AddProperty("emissive_color", gns::MaterialPropertyType::Color4);
        layout.AddProperty("texture_tiling_offset", gns::MaterialPropertyType::Vec4);
        layout.AddProperty("metallic", gns::MaterialPropertyType::Float);
        layout.AddProperty("roughness", gns::MaterialPropertyType::Float);
        layout.AddProperty("ambient_occlusion", gns::MaterialPropertyType::Float);
        layout.AddProperty("alpha", gns::MaterialPropertyType::Float);
        layout.AddProperty("normal_strength", gns::MaterialPropertyType::Float);
        layout.AddProperty("emissive_strength", gns::MaterialPropertyType::Float);
        layout.AddProperty("alpha_cutoff", gns::MaterialPropertyType::Float);
        return layout;
    }

    gns::MaterialLayout BuildImportedMaterialLayout()
    {
        gns::rendering::ShaderReflectionData fragmentReflection;
        const std::string fragmentShaderPath =
            gns::rendering::ShaderUtils::ResolveCompiledShaderPath(R"(Shaders\default.frag)");
        if (gns::rendering::ShaderUtils::ReflectShaderFile(fragmentShaderPath, fragmentReflection))
        {
            return gns::rendering::ShaderUtils::BuildMaterialLayout({ fragmentReflection });
        }

        LOG_WARNING("[AssetMetadataWriter]: Falling back to built-in material properties.");
        return BuildFallbackMaterialLayout();
    }

    bool WriteMaterialFile(
        const std::filesystem::path& materialPath,
        const MaterialArtifact& material,
        const gns::MaterialLayout& materialLayout)
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
        emitter << YAML::Key << "properties" << YAML::Value << YAML::BeginMap;
        for (const gns::MaterialPropertyInfo& property : materialLayout.GetProperties())
        {
            WriteMaterialProperty(emitter, material, property);
        }
        emitter << YAML::EndMap;
        emitter << YAML::Key << "textures" << YAML::Value << YAML::BeginMap;
        if (material.albedoTexture.IsValid())
        {
            emitter << YAML::Key << "albedo_texture" << YAML::Value << material.albedoTexture.Get();
        }
        if (material.normalMap.IsValid())
        {
            emitter << YAML::Key << "normal_map" << YAML::Value << material.normalMap.Get();
        }
        if (material.metallicMap.IsValid())
        {
            emitter << YAML::Key << "metallic_map" << YAML::Value << material.metallicMap.Get();
        }
        if (material.roughnessMap.IsValid())
        {
            emitter << YAML::Key << "roughness_map" << YAML::Value << material.roughnessMap.Get();
        }
        if (material.ambientOcclusionMap.IsValid())
        {
            emitter << YAML::Key << "ambient_occlusion_map" << YAML::Value << material.ambientOcclusionMap.Get();
        }
        if (material.emissiveMap.IsValid())
        {
            emitter << YAML::Key << "emissive_map" << YAML::Value << material.emissiveMap.Get();
        }
        emitter << YAML::EndMap;
        emitter << YAML::EndMap;

        return emitter.good() && gns::path::WriteTextFile(materialPath, emitter.c_str());
    }

    MaterialArtifact CreateMaterialArtifact(
        const aiMaterial* assimpMaterial,
        uint32_t materialIndex,
        const std::filesystem::path& materialPath,
        const std::string& sourcePath)
    {
        MaterialArtifact material
        {
            .handle = gns::assets::AssetManager::GetMaterialArtifactHandle(sourcePath, materialIndex),
            .materialIndex = materialIndex,
            .name = MaterialName(assimpMaterial, materialIndex),
            .path = ToProjectRelativeString(materialPath)
        };

        if (assimpMaterial == nullptr)
        {
            return material;
        }

        aiColor4D baseColor;
        if (assimpMaterial->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS ||
            assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor) == AI_SUCCESS)
        {
            material.albedoColor = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
            material.alpha = baseColor.a;
        }

        aiColor3D emissiveColor;
        if (assimpMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor) == AI_SUCCESS)
        {
            material.emissiveColor = glm::vec4(emissiveColor.r, emissiveColor.g, emissiveColor.b, 1.0f);
        }

        float value = 0.0f;
        if (assimpMaterial->Get(AI_MATKEY_METALLIC_FACTOR, value) == AI_SUCCESS)
        {
            material.metallic = value;
        }
        if (assimpMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, value) == AI_SUCCESS)
        {
            material.roughness = value;
        }
        if (assimpMaterial->Get(AI_MATKEY_OPACITY, value) == AI_SUCCESS)
        {
            material.alpha = value;
            material.albedoColor.a = value;
        }
        if (assimpMaterial->Get(AI_MATKEY_EMISSIVE_INTENSITY, value) == AI_SUCCESS)
        {
            material.emissiveStrength = value;
        }
        if (assimpMaterial->Get(AI_MATKEY_GLTF_ALPHACUTOFF, value) == AI_SUCCESS)
        {
            material.alphaCutoff = value;
        }
        if (assimpMaterial->Get(AI_MATKEY_GLTF_TEXTURE_STRENGTH(aiTextureType_NORMALS, 0), value) == AI_SUCCESS)
        {
            material.normalStrength = value;
        }

        return material;
    }

    std::filesystem::path ArtifactDirectory()
    {
        return gns::path::Resolve(gns::path::Root::ProjectLibrary, "Artifacts");
    }

    bool WriteArtifactLink(gns::Handle handle, const std::filesystem::path& targetPath)
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
        const std::string targetText = targetPath.is_absolute()
            ? ToProjectRelativeString(targetPath)
            : targetPath.generic_string();
        if (!gns::path::WriteTextFile(linkPath, targetText))
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
            WriteArtifactLink(material.handle, material.path);
        }

        for (const TextureArtifact& texture : textureArtifacts)
        {
            WriteArtifactLink(texture.handle, texture.path);
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
        const gns::MaterialLayout materialLayout = BuildImportedMaterialLayout();
        materialArtifacts.reserve(scene->mNumMaterials);
        const std::filesystem::path assetDirectory = gns::path::ParentDirectory(normalizedModelPath);
        for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
        {
            const aiMaterial* assimpMaterial = scene->mMaterials[materialIndex];
            const std::filesystem::path materialPath = MaterialFilePath(normalizedModelPath, materialIndex);
            MaterialArtifact material = CreateMaterialArtifact(
                assimpMaterial,
                materialIndex,
                materialPath,
                sourcePath);

            if (loadOptions.importTextures)
            {
                material.albedoTexture = CollectOptionalTexture(
                    assimpMaterial,
                    FindFirstMaterialTextureSlot(assimpMaterial),
                    assetDirectory,
                    sourcePath,
                    textureArtifacts);
                material.normalMap = CollectOptionalTexture(
                    assimpMaterial,
                    FindMaterialTextureSlot(assimpMaterial, aiTextureType_NORMALS),
                    assetDirectory,
                    sourcePath,
                    textureArtifacts);
                material.metallicMap = CollectOptionalTexture(
                    assimpMaterial,
                    FindMaterialTextureSlot(assimpMaterial, aiTextureType_METALNESS),
                    assetDirectory,
                    sourcePath,
                    textureArtifacts);
                material.roughnessMap = CollectOptionalTexture(
                    assimpMaterial,
                    FindMaterialTextureSlot(assimpMaterial, aiTextureType_DIFFUSE_ROUGHNESS),
                    assetDirectory,
                    sourcePath,
                    textureArtifacts);
                material.ambientOcclusionMap = CollectOptionalTexture(
                    assimpMaterial,
                    FindMaterialTextureSlot(assimpMaterial, aiTextureType_LIGHTMAP),
                    assetDirectory,
                    sourcePath,
                    textureArtifacts);
                material.emissiveMap = CollectOptionalTexture(
                    assimpMaterial,
                    FindMaterialTextureSlot(assimpMaterial, aiTextureType_EMISSIVE),
                    assetDirectory,
                    sourcePath,
                    textureArtifacts);
            }

            if (loadOptions.importMaterials && !WriteMaterialFile(materialPath, material, materialLayout))
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
