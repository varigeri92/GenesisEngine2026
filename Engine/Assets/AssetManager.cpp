#include "gnspch.h"
#include "AssetManager.h"
#include "assimp/Importer.hpp"
#include "assimp/material.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "stb_image.h"
#include "../Object/Material.h"
#include "../Object/Mesh.h"
#include "../Object/Texture.h"
#include "../Utils/Path.h"

#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <fstream>
#include <utility>

#include <glm/gtc/quaternion.hpp>
#include <yaml-cpp/yaml.h>

namespace
{
    struct AssetArtifactRecord
    {
        gns::assets::AssetArtifactType type = gns::assets::AssetArtifactType::Unknown;
        gns::Handle handle;
        std::filesystem::path sourcePath;
        gns::assets::AssetLoadOptions loadOptions;
        uint32_t index = 0;
        std::filesystem::path artifactPath;
    };

    struct TextureLoadResult
    {
        gns::Texture* texture = nullptr;
        bool textureSlotExists = false;
        bool failed = false;
    };

    struct NodeTransform
    {
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
    };

    std::unordered_map<gns::Handle, AssetArtifactRecord>& ArtifactRegistry()
    {
        static std::unordered_map<gns::Handle, AssetArtifactRecord> artifacts;
        return artifacts;
    }

    std::filesystem::path ArtifactDirectory()
    {
        return gns::path::Resolve(gns::path::Root::ProjectLibrary, "Artifacts");
    }

    std::filesystem::path ArtifactLinkPath(gns::Handle handle)
    {
        return ArtifactDirectory() / std::to_string(handle.Get());
    }

    std::string Trim(std::string value)
    {
        const char* whitespace = " \t\r\n";
        const size_t start = value.find_first_not_of(whitespace);
        if (start == std::string::npos)
        {
            return {};
        }

        const size_t end = value.find_last_not_of(whitespace);
        return value.substr(start, end - start + 1);
    }

    std::string ToProjectRelativeAssetString(const std::filesystem::path& assetPath)
    {
        const std::filesystem::path normalizedPath = gns::path::Normalize(assetPath);
        const std::filesystem::path projectRoot = gns::path::ProjectDirectory();
        if (projectRoot.empty())
        {
            return normalizedPath.generic_string();
        }

        return gns::path::ToRelative(normalizedPath, projectRoot).generic_string();
    }

    std::filesystem::path ResolveProjectPath(const std::filesystem::path& path)
    {
        return gns::path::Resolve(gns::path::Root::Project, path);
    }

    void RegisterArtifact(const AssetArtifactRecord& artifact)
    {
        if (artifact.handle.IsValid())
        {
            ArtifactRegistry()[artifact.handle] = artifact;
        }
    }

    gns::assets::AssetLoadOptions ReadLoadOptions(const YAML::Node& node)
    {
        gns::assets::AssetLoadOptions options;
        if (!node)
        {
            return options;
        }

        if (node["flattenHierarchy"])
        {
            options.flattenHierarchy = node["flattenHierarchy"].as<bool>();
        }
        if (node["importSkeleton"])
        {
            options.importSkeleton = node["importSkeleton"].as<bool>();
        }
        if (node["importMaterials"])
        {
            options.importMaterials = node["importMaterials"].as<bool>();
        }
        if (node["importTextures"])
        {
            options.importTextures = node["importTextures"].as<bool>();
        }

        return options;
    }

    void ReadArtifactSequence(
        const YAML::Node& sequence,
        gns::assets::AssetArtifactType type,
        const std::filesystem::path& sourcePath,
        const gns::assets::AssetLoadOptions& loadOptions)
    {
        if (!sequence || !sequence.IsSequence())
        {
            return;
        }

        for (const YAML::Node& artifactNode : sequence)
        {
            if (!artifactNode["handle"])
            {
                continue;
            }

            AssetArtifactRecord artifact;
            artifact.type = type;
            artifact.handle = gns::Handle::Create(artifactNode["handle"].as<uint64_t>());
            artifact.sourcePath = sourcePath;
            artifact.loadOptions = loadOptions;

            if (artifactNode["meshIndex"])
            {
                artifact.index = artifactNode["meshIndex"].as<uint32_t>();
            }
            else if (artifactNode["materialIndex"])
            {
                artifact.index = artifactNode["materialIndex"].as<uint32_t>();
            }

            if (artifactNode["path"])
            {
                artifact.artifactPath = artifactNode["path"].as<std::string>();
            }

            RegisterArtifact(artifact);
        }
    }

    bool LoadModelMetaFile(const std::filesystem::path& metaPath)
    {
        try
        {
            const YAML::Node root = YAML::LoadFile(metaPath.string());
            if (!root["sourcePath"])
            {
                return false;
            }

            const std::filesystem::path sourcePath = root["sourcePath"].as<std::string>();
            const gns::assets::AssetLoadOptions loadOptions = ReadLoadOptions(root["importOptions"]);
            const YAML::Node artifacts = root["artifacts"];
            if (!artifacts)
            {
                return false;
            }

            ReadArtifactSequence(artifacts["meshes"], gns::assets::AssetArtifactType::Mesh, sourcePath, loadOptions);
            ReadArtifactSequence(artifacts["materials"], gns::assets::AssetArtifactType::Material, sourcePath, loadOptions);
            ReadArtifactSequence(artifacts["textures"], gns::assets::AssetArtifactType::Texture, sourcePath, loadOptions);
            return true;
        }
        catch (const std::exception& exception)
        {
            LOG_WARNING("[AssetManager]: Failed to read asset metadata.");
            LOG_WARNING(metaPath.string());
            LOG_WARNING(exception.what());
            return false;
        }
    }

    bool LoadArtifactLink(gns::Handle handle)
    {
        const std::filesystem::path linkPath = ArtifactLinkPath(handle);
        if (!gns::path::IsRegularFile(linkPath))
        {
            return false;
        }

        std::ifstream file(linkPath);
        if (!file)
        {
            return false;
        }

        std::string metaPathText;
        std::getline(file, metaPathText, '\0');
        metaPathText = Trim(std::move(metaPathText));
        if (metaPathText.empty())
        {
            LOG_WARNING("[AssetManager]: Artifact link is empty.");
            LOG_WARNING(linkPath.string());
            return false;
        }

        const std::filesystem::path metaPath = ResolveProjectPath(metaPathText);
        if (!LoadModelMetaFile(metaPath))
        {
            LOG_WARNING("[AssetManager]: Artifact link points to unreadable metadata.");
            LOG_WARNING(metaPath.string());
            return false;
        }

        return ArtifactRegistry().contains(handle);
    }

    void LoadAssetRegistry()
    {
        ArtifactRegistry().clear();

        const std::filesystem::path assetsRoot = gns::path::AssetsDirectory();
        if (!gns::path::IsDirectory(assetsRoot))
        {
            return;
        }

        std::error_code error;
        for (const std::filesystem::directory_entry& entry :
            std::filesystem::recursive_directory_iterator(assetsRoot, error))
        {
            if (error)
            {
                break;
            }

            if (!entry.is_regular_file(error))
            {
                continue;
            }

            if (entry.path().extension() == ".meta")
            {
                LoadModelMetaFile(entry.path());
            }
        }
    }

    std::optional<AssetArtifactRecord> FindArtifact(gns::Handle handle)
    {
        auto& artifacts = ArtifactRegistry();
        if (const auto found = artifacts.find(handle); found != artifacts.end())
        {
            return found->second;
        }

        if (LoadArtifactLink(handle))
        {
            if (const auto found = artifacts.find(handle); found != artifacts.end())
            {
                return found->second;
            }
        }

        LoadAssetRegistry();
        if (const auto found = artifacts.find(handle); found != artifacts.end())
        {
            return found->second;
        }

        return std::nullopt;
    }

    void EnsureSourceAssetLoaded(const AssetArtifactRecord& artifact)
    {
        if (!artifact.sourcePath.empty())
        {
            gns::assets::AssetManager::LoadAsset(ResolveProjectPath(artifact.sourcePath).string(), artifact.loadOptions);
        }
    }

    const char* GetStbiFailureReason()
    {
        const char* reason = stbi_failure_reason();
        return reason != nullptr ? reason : "Unknown STB image failure.";
    }

    NodeTransform ToNodeTransform(const aiMatrix4x4& matrix)
    {
        aiVector3D scale;
        aiVector3D position;
        aiQuaternion rotation;
        matrix.Decompose(scale, rotation, position);

        const glm::quat glmRotation(rotation.w, rotation.x, rotation.y, rotation.z);
        return NodeTransform
        {
            .position = glm::vec3(position.x, position.y, position.z),
            .rotation = glm::degrees(glm::eulerAngles(glmRotation)),
            .scale = glm::vec3(scale.x, scale.y, scale.z)
        };
    }

    std::filesystem::path ResolveTexturePath(
        const std::filesystem::path& assetDirectory,
        const aiString& texturePath)
    {
        std::filesystem::path resolvedPath(texturePath.C_Str());
        return gns::path::ResolveAgainst(assetDirectory, resolvedPath);
    }

    gns::Texture* CreateTextureFromPixels(
        gns::Handle textureHandle,
        const std::string& name,
        const std::string& assetPath,
        std::vector<uint8_t> pixels,
        uint32_t width,
        uint32_t height,
        std::unordered_map<std::string, gns::Texture*>& textureCache)
    {
        if (pixels.empty() || width == 0 || height == 0)
        {
            LOG_ERROR("[AssetManager]: Cannot create texture from empty pixel data.");
            LOG_ERROR(name);
            return nullptr;
        }

        if (const auto cachedTexture = textureCache.find(name); cachedTexture != textureCache.end())
        {
            return cachedTexture->second;
        }

        gns::Texture* texture = gns::Object::Create<gns::Texture>(textureHandle, name, assetPath);
        if (texture == nullptr)
        {
            LOG_ERROR("[AssetManager]: Failed to create texture object.");
            LOG_ERROR(name);
            return nullptr;
        }

        texture->SetPixels(std::move(pixels), width, height, 4, gns::TextureFormat::R8G8B8A8_UNorm);
        textureCache[name] = texture;
        return texture;
    }

    gns::Texture* LoadTextureFromFile(
        const std::filesystem::path& texturePath,
        std::unordered_map<std::string, gns::Texture*>& textureCache)
    {
        const std::string textureAssetPath = ToProjectRelativeAssetString(texturePath);
        if (const auto cachedTexture = textureCache.find(textureAssetPath); cachedTexture != textureCache.end())
        {
            return cachedTexture->second;
        }

        const std::string normalizedPath = gns::path::Normalize(texturePath).string();
        if (!gns::path::Exists(texturePath))
        {
            LOG_ERROR("[AssetManager]: Texture file does not exist.");
            LOG_ERROR(normalizedPath);
            return nullptr;
        }

        int width = 0;
        int height = 0;
        int sourceChannels = 0;
        stbi_uc* loadedPixels = stbi_load(normalizedPath.c_str(), &width, &height, &sourceChannels, 4);
        if (loadedPixels == nullptr)
        {
            LOG_ERROR("[AssetManager]: Failed to load texture file.");
            LOG_ERROR(normalizedPath);
            LOG_ERROR(GetStbiFailureReason());
            return nullptr;
        }

        const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
        std::vector<uint8_t> pixels(loadedPixels, loadedPixels + pixelCount);
        stbi_image_free(loadedPixels);

        return CreateTextureFromPixels(
            gns::assets::AssetManager::GetTextureArtifactHandle(textureAssetPath),
            textureAssetPath,
            textureAssetPath,
            std::move(pixels),
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            textureCache);
    }

    gns::Texture* LoadEmbeddedTexture(
        const aiScene* scene,
        const aiString& texturePath,
        const std::string& assetPath,
        std::unordered_map<std::string, gns::Texture*>& textureCache)
    {
        const char* textureName = texturePath.C_Str();
        if (textureName == nullptr || textureName[0] != '*')
        {
            return nullptr;
        }

        const int textureIndex = std::atoi(textureName + 1);
        if (textureIndex < 0 || static_cast<uint32_t>(textureIndex) >= scene->mNumTextures)
        {
            LOG_ERROR("[AssetManager]: Embedded texture index is invalid.");
            LOG_ERROR(textureName);
            return nullptr;
        }

        const std::string textureObjectName =
            assetPath + "::embedded_texture_" + std::to_string(textureIndex);
        if (const auto cachedTexture = textureCache.find(textureObjectName); cachedTexture != textureCache.end())
        {
            return cachedTexture->second;
        }

        const aiTexture* embeddedTexture = scene->mTextures[textureIndex];
        if (embeddedTexture == nullptr)
        {
            LOG_ERROR("[AssetManager]: Embedded texture is missing.");
            LOG_ERROR(textureName);
            return nullptr;
        }

        if (embeddedTexture->mHeight == 0)
        {
            int width = 0;
            int height = 0;
            int sourceChannels = 0;
            stbi_uc* loadedPixels = stbi_load_from_memory(
                reinterpret_cast<const stbi_uc*>(embeddedTexture->pcData),
                static_cast<int>(embeddedTexture->mWidth),
                &width,
                &height,
                &sourceChannels,
                4);

            if (loadedPixels == nullptr)
            {
                LOG_ERROR("[AssetManager]: Failed to load compressed embedded texture.");
                LOG_ERROR(textureObjectName);
                LOG_ERROR(GetStbiFailureReason());
                return nullptr;
            }

            const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
            std::vector<uint8_t> pixels(loadedPixels, loadedPixels + pixelCount);
            stbi_image_free(loadedPixels);

            return CreateTextureFromPixels(
                gns::assets::AssetManager::GetTextureArtifactHandle(textureObjectName),
                textureObjectName,
                textureObjectName,
                std::move(pixels),
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height),
                textureCache);
        }

        const size_t texelCount =
            static_cast<size_t>(embeddedTexture->mWidth) * static_cast<size_t>(embeddedTexture->mHeight);
        std::vector<uint8_t> pixels(texelCount * 4);
        for (size_t i = 0; i < texelCount; ++i)
        {
            const aiTexel& texel = embeddedTexture->pcData[i];
            pixels[i * 4 + 0] = texel.r;
            pixels[i * 4 + 1] = texel.g;
            pixels[i * 4 + 2] = texel.b;
            pixels[i * 4 + 3] = texel.a;
        }

        return CreateTextureFromPixels(
            gns::assets::AssetManager::GetTextureArtifactHandle(textureObjectName),
            textureObjectName,
            textureObjectName,
            std::move(pixels),
            embeddedTexture->mWidth,
            embeddedTexture->mHeight,
            textureCache);
    }

    TextureLoadResult LoadMaterialTexture(
        const aiScene* scene,
        const aiMaterial* material,
        aiTextureType textureType,
        const std::filesystem::path& assetDirectory,
        const std::string& assetPath,
        std::unordered_map<std::string, gns::Texture*>& textureCache)
    {
        if (material->GetTextureCount(textureType) == 0)
        {
            return {};
        }

        aiString texturePath;
        if (material->GetTexture(textureType, 0, &texturePath) != AI_SUCCESS)
        {
            return { nullptr, true, true };
        }

        if (texturePath.length == 0)
        {
            return { nullptr, true, true };
        }

        gns::Texture* texture = nullptr;
        if (texturePath.C_Str()[0] == '*')
        {
            texture = LoadEmbeddedTexture(scene, texturePath, assetPath, textureCache);
        }
        else
        {
            texture = LoadTextureFromFile(ResolveTexturePath(assetDirectory, texturePath), textureCache);
        }

        return { texture, true, texture == nullptr };
    }

    gns::Material* CreateMaterial(
        const aiScene* scene,
        const aiMaterial* assimpMaterial,
        uint32_t materialIndex,
        const std::filesystem::path& assetDirectory,
        const std::string& assetPath,
        std::unordered_map<std::string, gns::Texture*>& textureCache,
        bool importTextures)
    {
        const std::string materialName =
            assetPath + "::material_" + std::to_string(materialIndex) + "_" + assimpMaterial->GetName().C_Str();
        const gns::Handle materialHandle =
            gns::assets::AssetManager::GetMaterialArtifactHandle(assetPath, materialIndex);
        gns::Material* material = gns::Object::Create<gns::Material>(materialHandle, materialName);
        if (material == nullptr)
        {
            LOG_WARNING("[AssetManager]: Failed to create material object.");
            LOG_WARNING(materialName);
            return nullptr;
        }

        aiColor4D baseColor;
        if (aiGetMaterialColor(assimpMaterial, AI_MATKEY_BASE_COLOR, &baseColor) == AI_SUCCESS ||
            aiGetMaterialColor(assimpMaterial, AI_MATKEY_COLOR_DIFFUSE, &baseColor) == AI_SUCCESS)
        {
            material->albedo_color = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
            material->SetVec4("albedo_color", material->albedo_color);
        }

        if (importTextures)
        {
            TextureLoadResult albedoTexture = LoadMaterialTexture(
                scene,
                assimpMaterial,
                aiTextureType_BASE_COLOR,
                assetDirectory,
                assetPath,
                textureCache);
            if (!albedoTexture.textureSlotExists)
            {
                albedoTexture = LoadMaterialTexture(
                    scene,
                    assimpMaterial,
                    aiTextureType_DIFFUSE,
                    assetDirectory,
                    assetPath,
                    textureCache);
            }

            if (albedoTexture.texture != nullptr)
            {
                material->albedo_texture = albedoTexture.texture->Ref<gns::Texture>();
            }
            else if (albedoTexture.failed)
            {
                material->albedo_texture = gns::Reference<gns::Texture>(
                    gns::Handle::CreateFromString(gns::DefaultResourceNames::ErrorCheckerboardTexture));
            }
        }

        return material;
    }
}

namespace
{
    gns::assets::LoadedObject LoadMesh(
        const aiMesh* mesh,
        uint32_t meshIndex,
        const std::string& sourcePath,
        const std::vector<gns::Handle>& materialHandles,
        const NodeTransform& transform)
    {
        std::string meshName = mesh->mName.C_Str();
        if (meshName.empty())
        {
            meshName = "Mesh_" + std::to_string(meshIndex);
        }

        LOG_INFO(meshName);
        const gns::Handle meshHandle = gns::assets::AssetManager::GetMeshArtifactHandle(sourcePath, meshIndex);
        gns::Mesh* newMesh = gns::Object::Create<gns::Mesh>(meshHandle, meshName);
        if (newMesh == nullptr)
        {
            LOG_WARNING("[AssetManager]: Failed to create mesh object.");
            LOG_WARNING(meshName);
            return {};
        }

        gns::Handle materialHandle;
        if (mesh->mMaterialIndex < materialHandles.size())
        {
            materialHandle = materialHandles[mesh->mMaterialIndex];
        }

        for (size_t v = 0; v < mesh->mNumVertices; v++)
        {
            newMesh->positions.emplace_back(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);
            if (mesh->HasNormals())
            {
                newMesh->normals.emplace_back(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
                newMesh->colors.emplace_back(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z, 1.f);
            }
            else
            {
                newMesh->normals.emplace_back(0.0f, 1.0f, 0.0f);
                newMesh->colors.emplace_back(1.0f);
            }

            if (mesh->HasTextureCoords(0))
            {
                newMesh->uvs.emplace_back(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y * -1);
            }
            else
            {
                newMesh->uvs.emplace_back(0.0f);
            }
        }

        if (mesh->HasTangentsAndBitangents())
        {
            for (size_t v = 0; v < mesh->mNumVertices; v++)
            {
                newMesh->tangents.emplace_back(mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z);
                newMesh->bitangents.emplace_back(mesh->mBitangents[v].x, mesh->mBitangents[v].y, mesh->mBitangents[v].z);
            }
        }

        const size_t startindex = newMesh->indices.size();
        for (uint32_t f = 0; f < mesh->mNumFaces; f++)
        {
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

        return
        {
            .objectHandle = newMesh->GetHandle(),
            .object = newMesh,
            .materialHandle = materialHandle,
            .position = transform.position,
            .rotation = transform.rotation,
            .scale = transform.scale
        };
    }

    void LoadMeshesFromNode(
        const aiScene* scene,
        const aiNode* node,
        const aiMatrix4x4& parentTransform,
        const std::string& sourcePath,
        const std::vector<gns::Handle>& materialHandles,
        bool flattenHierarchy,
        std::vector<gns::assets::LoadedObject>& loaded)
    {
        const aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;
        const NodeTransform transform = flattenHierarchy ? NodeTransform{} : ToNodeTransform(nodeTransform);

        for (uint32_t meshSlot = 0; meshSlot < node->mNumMeshes; ++meshSlot)
        {
            const uint32_t meshIndex = node->mMeshes[meshSlot];
            if (meshIndex >= scene->mNumMeshes || scene->mMeshes[meshIndex] == nullptr)
            {
                continue;
            }

            gns::assets::LoadedObject loadedMesh =
                LoadMesh(scene->mMeshes[meshIndex], meshIndex, sourcePath, materialHandles, transform);
            if (loadedMesh.object != nullptr)
            {
                loaded.emplace_back(loadedMesh);
            }
        }

        for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
        {
            if (node->mChildren[childIndex] != nullptr)
            {
                LoadMeshesFromNode(
                    scene,
                    node->mChildren[childIndex],
                    nodeTransform,
                    sourcePath,
                    materialHandles,
                    flattenHierarchy,
                    loaded);
            }
        }
    }
}

std::vector<gns::assets::LoadedObject> gns::assets::AssetManager::LoadAsset(const std::string& path)
{
    return LoadAsset(path, AssetLoadOptions{});
}

std::vector<gns::assets::LoadedObject> gns::assets::AssetManager::LoadAsset(
    const std::string& path,
    const AssetLoadOptions& loadOptions)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
      aiProcess_CalcTangentSpace       |
      aiProcess_Triangulate            |
      aiProcess_JoinIdenticalVertices  |
      aiProcess_SortByPType);
    if (nullptr == scene) {
        LOG_ERROR(importer.GetErrorString());
        return {};
    }
    if (scene->HasMeshes())
    {
        std::vector<gns::assets::LoadedObject> loaded;
        loaded.reserve(scene->mNumMeshes);

        const std::filesystem::path assetPath = gns::path::Normalize(path);
        const std::string sourcePath = ToProjectRelativeAssetString(assetPath);
        const std::filesystem::path assetDirectory = gns::path::ParentDirectory(assetPath);
        std::unordered_map<std::string, gns::Texture*> textureCache;
        std::vector<gns::Handle> materialHandles;

        if (loadOptions.importMaterials && scene->HasMaterials())
        {
            materialHandles.reserve(scene->mNumMaterials);
            for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
            {
                gns::Material* material = CreateMaterial(
                    scene,
                    scene->mMaterials[materialIndex],
                    materialIndex,
                    assetDirectory,
                    sourcePath,
                    textureCache,
                    loadOptions.importTextures);
                materialHandles.push_back(material != nullptr ? material->GetHandle() : gns::Handle{});
            }
        }

        if (scene->mRootNode != nullptr)
        {
            LoadMeshesFromNode(
                scene,
                scene->mRootNode,
                aiMatrix4x4(),
                sourcePath,
                materialHandles,
                loadOptions.flattenHierarchy,
                loaded);
        }
        else
        {
            const NodeTransform identityTransform = {};
            for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
            {
                gns::assets::LoadedObject loadedMesh =
                    LoadMesh(scene->mMeshes[meshIndex], meshIndex, sourcePath, materialHandles, identityTransform);
                if (loadedMesh.object != nullptr)
                {
                    loaded.emplace_back(loadedMesh);
                }
            }
        }

        return loaded;
    }
    LOG_INFO("File: " + path + " does not contain meshes.");
    return {};
}

gns::Mesh* gns::assets::AssetManager::EnsureMeshLoaded(Handle meshHandle)
{
    if (gns::Mesh* mesh = Object::Get<gns::Mesh>(meshHandle))
    {
        return mesh;
    }

    const std::optional<AssetArtifactRecord> artifact = FindArtifact(meshHandle);
    if (!artifact || artifact->type != AssetArtifactType::Mesh)
    {
        LOG_WARNING("[AssetManager]: Cannot resolve mesh artifact handle.");
        LOG_WARNING(std::to_string(meshHandle.Get()));
        return nullptr;
    }

    EnsureSourceAssetLoaded(*artifact);
    return Object::Get<gns::Mesh>(meshHandle);
}

gns::Material* gns::assets::AssetManager::EnsureMaterialLoaded(Handle materialHandle)
{
    if (gns::Material* material = Object::Get<gns::Material>(materialHandle))
    {
        return material;
    }

    const std::optional<AssetArtifactRecord> artifact = FindArtifact(materialHandle);
    if (!artifact || artifact->type != AssetArtifactType::Material)
    {
        LOG_WARNING("[AssetManager]: Cannot resolve material artifact handle.");
        LOG_WARNING(std::to_string(materialHandle.Get()));
        return nullptr;
    }

    EnsureSourceAssetLoaded(*artifact);
    return Object::Get<gns::Material>(materialHandle);
}

gns::Texture* gns::assets::AssetManager::EnsureTextureLoaded(Handle textureHandle)
{
    if (gns::Texture* texture = Object::Get<gns::Texture>(textureHandle))
    {
        return texture;
    }

    const std::optional<AssetArtifactRecord> artifact = FindArtifact(textureHandle);
    if (artifact)
    {
        const std::string artifactPath = artifact->artifactPath.generic_string();
        const bool isEmbeddedTexture = artifactPath.find("::embedded_texture_") != std::string::npos;
        if (artifact->type == AssetArtifactType::Texture && !artifact->artifactPath.empty() && !isEmbeddedTexture)
        {
            std::unordered_map<std::string, gns::Texture*> textureCache;
            return LoadTextureFromFile(ResolveProjectPath(artifact->artifactPath), textureCache);
        }

        EnsureSourceAssetLoaded(*artifact);
        return Object::Get<gns::Texture>(textureHandle);
    }

    LOG_WARNING("[AssetManager]: Cannot resolve texture artifact handle.");
    LOG_WARNING(std::to_string(textureHandle.Get()));
    return nullptr;
}

gns::Handle gns::assets::AssetManager::GetModelAssetHandle(const std::filesystem::path& sourcePath)
{
    return gns::Handle::CreateFromString("asset:" + sourcePath.generic_string());
}

gns::Handle gns::assets::AssetManager::GetMeshArtifactHandle(
    const std::filesystem::path& sourcePath,
    uint32_t meshIndex)
{
    return gns::Handle::CreateFromString(
        "mesh:" + sourcePath.generic_string() + ":" + std::to_string(meshIndex));
}

gns::Handle gns::assets::AssetManager::GetMaterialArtifactHandle(
    const std::filesystem::path& sourcePath,
    uint32_t materialIndex)
{
    return gns::Handle::CreateFromString(
        "material:" + sourcePath.generic_string() + ":" + std::to_string(materialIndex));
}

gns::Handle gns::assets::AssetManager::GetTextureArtifactHandle(const std::filesystem::path& texturePath)
{
    return gns::Handle::CreateFromString("texture:" + texturePath.generic_string());
}
