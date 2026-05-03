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
#include <utility>

#include <glm/gtc/quaternion.hpp>

std::unordered_map<gns::Handle, gns::assets::Asset> gns::assets::AssetManager::AssetMap = {};

namespace
{
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

        gns::Texture* texture = gns::Object::Create<gns::Texture>(name, assetPath);
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
        const std::string normalizedPath = gns::path::Normalize(texturePath).string();
        if (const auto cachedTexture = textureCache.find(normalizedPath); cachedTexture != textureCache.end())
        {
            return cachedTexture->second;
        }

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
            normalizedPath,
            normalizedPath,
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
        gns::Material* material = gns::Object::Create<gns::Material>(materialName);
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
        const std::vector<gns::Handle>& materialHandles,
        const NodeTransform& transform)
    {
        std::string meshName = mesh->mName.C_Str();
        if (meshName.empty())
        {
            meshName = "Mesh_" + std::to_string(meshIndex);
        }

        LOG_INFO(meshName);
        gns::Mesh* newMesh = gns::Object::Create<gns::Mesh>(meshName);
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
                LoadMesh(scene->mMeshes[meshIndex], meshIndex, materialHandles, transform);
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
                    path,
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
                    LoadMesh(scene->mMeshes[meshIndex], meshIndex, materialHandles, identityTransform);
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
