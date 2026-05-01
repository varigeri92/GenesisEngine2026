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
#include "../Utils/FileSystemUtils.h"

#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <utility>

std::unordered_map<gns::Handle, gns::assets::Asset> gns::assets::AssetManager::AssetMap = {};

namespace
{
    const char* GetStbiFailureReason()
    {
        const char* reason = stbi_failure_reason();
        return reason != nullptr ? reason : "Unknown STB image failure.";
    }

    std::filesystem::path ResolveTexturePath(
        const std::filesystem::path& assetDirectory,
        const aiString& texturePath)
    {
        std::filesystem::path resolvedPath(texturePath.C_Str());
        if (!resolvedPath.is_absolute())
        {
            resolvedPath = assetDirectory / resolvedPath;
        }

        return resolvedPath.lexically_normal();
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
            LOG_WARNING("[AssetManager]: Cannot create texture from empty pixel data.");
            LOG_WARNING(name);
            return nullptr;
        }

        if (const auto cachedTexture = textureCache.find(name); cachedTexture != textureCache.end())
        {
            return cachedTexture->second;
        }

        gns::Texture* texture = gns::Object::Create<gns::Texture>(name, assetPath);
        if (texture == nullptr)
        {
            LOG_WARNING("[AssetManager]: Failed to create texture object.");
            LOG_WARNING(name);
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
        const std::string normalizedPath = texturePath.lexically_normal().string();
        if (const auto cachedTexture = textureCache.find(normalizedPath); cachedTexture != textureCache.end())
        {
            return cachedTexture->second;
        }

        if (!std::filesystem::exists(texturePath))
        {
            LOG_WARNING("[AssetManager]: Texture file does not exist.");
            LOG_WARNING(normalizedPath);
            return nullptr;
        }

        int width = 0;
        int height = 0;
        int sourceChannels = 0;
        stbi_uc* loadedPixels = stbi_load(normalizedPath.c_str(), &width, &height, &sourceChannels, 4);
        if (loadedPixels == nullptr)
        {
            LOG_WARNING("[AssetManager]: Failed to load texture file.");
            LOG_WARNING(normalizedPath);
            LOG_WARNING(GetStbiFailureReason());
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
            LOG_WARNING("[AssetManager]: Embedded texture index is invalid.");
            LOG_WARNING(textureName);
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
            LOG_WARNING("[AssetManager]: Embedded texture is missing.");
            LOG_WARNING(textureName);
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
                LOG_WARNING("[AssetManager]: Failed to load compressed embedded texture.");
                LOG_WARNING(textureObjectName);
                LOG_WARNING(GetStbiFailureReason());
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

    gns::Texture* LoadMaterialTexture(
        const aiScene* scene,
        const aiMaterial* material,
        aiTextureType textureType,
        const std::filesystem::path& assetDirectory,
        const std::string& assetPath,
        std::unordered_map<std::string, gns::Texture*>& textureCache)
    {
        if (material->GetTextureCount(textureType) == 0)
        {
            return nullptr;
        }

        aiString texturePath;
        if (material->GetTexture(textureType, 0, &texturePath) != AI_SUCCESS)
        {
            return nullptr;
        }

        if (texturePath.length == 0)
        {
            return nullptr;
        }

        if (texturePath.C_Str()[0] == '*')
        {
            return LoadEmbeddedTexture(scene, texturePath, assetPath, textureCache);
        }

        return LoadTextureFromFile(ResolveTexturePath(assetDirectory, texturePath), textureCache);
    }

    gns::Material* CreateMaterial(
        const aiScene* scene,
        const aiMaterial* assimpMaterial,
        uint32_t materialIndex,
        const std::filesystem::path& assetDirectory,
        const std::string& assetPath,
        std::unordered_map<std::string, gns::Texture*>& textureCache)
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

        gns::Texture* albedoTexture = LoadMaterialTexture(
            scene,
            assimpMaterial,
            aiTextureType_BASE_COLOR,
            assetDirectory,
            assetPath,
            textureCache);
        if (albedoTexture == nullptr)
        {
            albedoTexture = LoadMaterialTexture(
                scene,
                assimpMaterial,
                aiTextureType_DIFFUSE,
                assetDirectory,
                assetPath,
                textureCache);
        }

        if (albedoTexture != nullptr)
        {
            material->albedo_texture = albedoTexture->Ref<gns::Texture>();
        }

        return material;
    }
}

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
        loaded.reserve(scene->mNumMeshes);

        const std::filesystem::path assetPath(path);
        const std::filesystem::path assetDirectory = assetPath.parent_path();
        std::unordered_map<std::string, gns::Texture*> textureCache;
        std::vector<gns::Handle> materialHandles;

        if (scene->HasMaterials())
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
                    textureCache);
                materialHandles.push_back(material != nullptr ? material->GetHandle() : gns::Handle{});
            }
        }

        for (uint32_t m = 0; m < scene->mNumMeshes; m++)
        {
            LOG_INFO(scene->mMeshes[m]->mName.C_Str());
            const aiMesh* mesh = scene->mMeshes[m];
            gns::Mesh* newMesh = gns::Object::Create<gns::Mesh>(mesh->mName.C_Str());

            if (newMesh == nullptr)
            {
                LOG_WARNING("[AssetManager]: Failed to create mesh object.");
                LOG_WARNING(mesh->mName.C_Str());
                continue;
            }

            gns::Handle materialHandle;
            if (mesh->mMaterialIndex < materialHandles.size())
            {
                materialHandle = materialHandles[mesh->mMaterialIndex];
            }
                
            for (size_t v = 0; v < scene->mMeshes[m]->mNumVertices; v++)
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
            loaded.push_back({ newMesh->GetHandle(), newMesh, materialHandle });
        }
        return loaded;
    }
    LOG_INFO("File: " + path + " does not contain meshes.");
    return {};
}
