#include "gnspch.h"
#include "AssetLoader.h"

#include "assimp/Importer.hpp"
#include "assimp/material.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "stb_image.h"
#include "../Utils/Path.h"

#include <array>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <unordered_set>

#include <glm/gtc/quaternion.hpp>

namespace
{
    struct NodeTransform
    {
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
    };

    struct MaterialTextureSlot
    {
        aiTextureType type = aiTextureType_NONE;
        uint32_t index = 0;
    };

    const char* GetStbiFailureReason()
    {
        const char* reason = stbi_failure_reason();
        return reason != nullptr ? reason : "Unknown STB image failure.";
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

    gns::Handle GetMeshArtifactHandle(
        const std::filesystem::path& sourcePath,
        uint32_t meshIndex)
    {
        return gns::Handle::CreateFromString(
            "mesh:" + sourcePath.generic_string() + ":" + std::to_string(meshIndex));
    }

    gns::Handle GetMaterialArtifactHandle(
        const std::filesystem::path& sourcePath,
        uint32_t materialIndex)
    {
        return gns::Handle::CreateFromString(
            "material:" + sourcePath.generic_string() + ":" + std::to_string(materialIndex));
    }

    gns::Handle GetTextureArtifactHandle(const std::filesystem::path& texturePath)
    {
        return gns::Handle::CreateFromString("texture:" + texturePath.generic_string());
    }

    bool IsTextureSourcePath(const std::filesystem::path& path)
    {
        std::string extension = gns::path::Extension(path);
        for (char& c : extension)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        return extension == "png" ||
            extension == "jpg" ||
            extension == "jpeg" ||
            extension == "tga" ||
            extension == "bmp";
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

    std::optional<MaterialTextureSlot> FindFirstMaterialTextureSlot(
        const aiMaterial* material)
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

    std::optional<gns::assets::TextureAssetData> DecodeTextureFile(
        const std::filesystem::path& texturePath)
    {
        const std::string textureAssetPath = ToProjectRelativeAssetString(texturePath);
        const std::string normalizedPath = gns::path::Normalize(texturePath).string();
        if (!gns::path::Exists(texturePath))
        {
            LOG_ERROR("[AssetLoader]: Texture file does not exist.");
            LOG_ERROR(normalizedPath);
            return std::nullopt;
        }

        int width = 0;
        int height = 0;
        int sourceChannels = 0;
        stbi_uc* loadedPixels = stbi_load(normalizedPath.c_str(), &width, &height, &sourceChannels, 4);
        if (loadedPixels == nullptr)
        {
            LOG_ERROR("[AssetLoader]: Failed to load texture file.");
            LOG_ERROR(normalizedPath);
            LOG_ERROR(GetStbiFailureReason());
            return std::nullopt;
        }

        const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
        std::vector<uint8_t> pixels(loadedPixels, loadedPixels + pixelCount);
        stbi_image_free(loadedPixels);

        gns::assets::TextureAssetData texture;
        texture.handle = GetTextureArtifactHandle(textureAssetPath);
        texture.name = textureAssetPath;
        texture.assetPath = textureAssetPath;
        texture.width = static_cast<uint32_t>(width);
        texture.height = static_cast<uint32_t>(height);
        texture.channels = 4;
        texture.format = gns::assets::AssetTextureFormat::R8G8B8A8_UNorm;
        texture.pixels = std::move(pixels);
        return texture;
    }

    std::optional<gns::assets::TextureAssetData> DecodeEmbeddedTexture(
        const aiScene* scene,
        const aiString& texturePath,
        const std::string& assetPath)
    {
        const char* textureName = texturePath.C_Str();
        if (scene == nullptr || textureName == nullptr || textureName[0] != '*')
        {
            return std::nullopt;
        }

        const int textureIndex = std::atoi(textureName + 1);
        if (textureIndex < 0 || static_cast<uint32_t>(textureIndex) >= scene->mNumTextures)
        {
            LOG_ERROR("[AssetLoader]: Embedded texture index is invalid.");
            LOG_ERROR(textureName);
            return std::nullopt;
        }

        const std::string textureObjectName =
            assetPath + "::embedded_texture_" + std::to_string(textureIndex);
        const aiTexture* embeddedTexture = scene->mTextures[textureIndex];
        if (embeddedTexture == nullptr)
        {
            LOG_ERROR("[AssetLoader]: Embedded texture is missing.");
            LOG_ERROR(textureName);
            return std::nullopt;
        }

        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<uint8_t> pixels;
        if (embeddedTexture->mHeight == 0)
        {
            int decodedWidth = 0;
            int decodedHeight = 0;
            int sourceChannels = 0;
            stbi_uc* loadedPixels = stbi_load_from_memory(
                reinterpret_cast<const stbi_uc*>(embeddedTexture->pcData),
                static_cast<int>(embeddedTexture->mWidth),
                &decodedWidth,
                &decodedHeight,
                &sourceChannels,
                4);

            if (loadedPixels == nullptr)
            {
                LOG_ERROR("[AssetLoader]: Failed to load compressed embedded texture.");
                LOG_ERROR(textureObjectName);
                LOG_ERROR(GetStbiFailureReason());
                return std::nullopt;
            }

            const size_t pixelCount = static_cast<size_t>(decodedWidth) * static_cast<size_t>(decodedHeight) * 4;
            pixels.assign(loadedPixels, loadedPixels + pixelCount);
            stbi_image_free(loadedPixels);
            width = static_cast<uint32_t>(decodedWidth);
            height = static_cast<uint32_t>(decodedHeight);
        }
        else
        {
            width = embeddedTexture->mWidth;
            height = embeddedTexture->mHeight;
            const size_t texelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
            pixels.resize(texelCount * 4);
            for (size_t i = 0; i < texelCount; ++i)
            {
                const aiTexel& texel = embeddedTexture->pcData[i];
                pixels[i * 4 + 0] = texel.r;
                pixels[i * 4 + 1] = texel.g;
                pixels[i * 4 + 2] = texel.b;
                pixels[i * 4 + 3] = texel.a;
            }
        }

        gns::assets::TextureAssetData texture;
        texture.handle = GetTextureArtifactHandle(textureObjectName);
        texture.name = textureObjectName;
        texture.assetPath = textureObjectName;
        texture.width = width;
        texture.height = height;
        texture.channels = 4;
        texture.format = gns::assets::AssetTextureFormat::R8G8B8A8_UNorm;
        texture.pixels = std::move(pixels);
        return texture;
    }

    void AddTextureDescription(
        gns::assets::AssetLoadResult& result,
        gns::assets::TextureAssetData textureData,
        std::unordered_set<gns::Handle>& loadedTextures)
    {
        if (!textureData.handle.IsValid() || loadedTextures.contains(textureData.handle))
        {
            return;
        }

        const gns::Handle textureHandle = textureData.handle;
        loadedTextures.insert(textureHandle);
        result.assets.emplace_back(gns::assets::AssetDescription
        {
            .type = gns::assets::Texture,
            .handle = textureHandle,
            .payload = std::move(textureData)
        });
    }

    std::optional<gns::assets::TextureAssetData> LoadMaterialTextureData(
        const aiScene* scene,
        const aiMaterial* material,
        const MaterialTextureSlot& textureSlot,
        const std::filesystem::path& assetDirectory,
        const std::string& assetPath)
    {
        aiString texturePath;
        if (material == nullptr ||
            material->GetTexture(textureSlot.type, textureSlot.index, &texturePath) != AI_SUCCESS ||
            texturePath.length == 0)
        {
            return std::nullopt;
        }

        if (texturePath.C_Str()[0] == '*')
        {
            return DecodeEmbeddedTexture(scene, texturePath, assetPath);
        }

        return DecodeTextureFile(ResolveTexturePath(assetDirectory, texturePath));
    }

    std::optional<gns::Handle> GetMaterialTextureHandle(
        const aiMaterial* material,
        const MaterialTextureSlot& textureSlot,
        const std::filesystem::path& assetDirectory,
        const std::string& assetPath)
    {
        aiString texturePath;
        if (material == nullptr ||
            material->GetTexture(textureSlot.type, textureSlot.index, &texturePath) != AI_SUCCESS ||
            texturePath.length == 0)
        {
            return std::nullopt;
        }

        if (texturePath.C_Str()[0] == '*')
        {
            const int textureIndex = std::atoi(texturePath.C_Str() + 1);
            if (textureIndex < 0)
            {
                return std::nullopt;
            }

            return GetTextureArtifactHandle(
                assetPath + "::embedded_texture_" + std::to_string(textureIndex));
        }

        const std::string textureAssetPath =
            ToProjectRelativeAssetString(ResolveTexturePath(assetDirectory, texturePath));
        return GetTextureArtifactHandle(textureAssetPath);
    }

    std::string MaterialName(const aiMaterial* material, uint32_t materialIndex)
    {
        if (material != nullptr && material->GetName().length > 0)
        {
            return material->GetName().C_Str();
        }

        return "Material_" + std::to_string(materialIndex);
    }

    gns::assets::MaterialAssetData LoadMaterialData(
        const aiMaterial* material,
        uint32_t materialIndex,
        const std::filesystem::path& assetDirectory,
        const std::string& sourcePath,
        bool importTextures)
    {
        gns::assets::MaterialAssetData materialData;
        materialData.handle = GetMaterialArtifactHandle(sourcePath, materialIndex);
        materialData.name =
            sourcePath + "::material_" + std::to_string(materialIndex) + "_" + MaterialName(material, materialIndex);

        if (importTextures)
        {
            const std::optional<MaterialTextureSlot> textureSlot = FindFirstMaterialTextureSlot(material);
            if (textureSlot)
            {
                const std::optional<gns::Handle> textureHandle =
                    GetMaterialTextureHandle(material, *textureSlot, assetDirectory, sourcePath);
                if (textureHandle && textureHandle->IsValid())
                {
                    materialData.textures.emplace_back(gns::assets::MaterialTextureReference
                    {
                        .propertyName = "albedo_texture",
                        .textureHandle = *textureHandle
                    });
                }
            }
        }

        return materialData;
    }

    void AddMaterialDescription(
        gns::assets::AssetLoadResult& result,
        gns::assets::MaterialAssetData materialData)
    {
        if (!materialData.handle.IsValid())
        {
            return;
        }

        const gns::Handle materialHandle = materialData.handle;
        result.assets.emplace_back(gns::assets::AssetDescription
        {
            .type = gns::assets::Material,
            .handle = materialHandle,
            .payload = std::move(materialData)
        });
    }

    void LoadMaterials(
        const aiScene* scene,
        const std::filesystem::path& assetDirectory,
        const std::string& sourcePath,
        bool importTextures,
        gns::assets::AssetLoadResult& result,
        std::vector<gns::Handle>& materialHandles)
    {
        if (scene == nullptr || !scene->HasMaterials())
        {
            return;
        }

        materialHandles.reserve(scene->mNumMaterials);
        for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
        {
            gns::assets::MaterialAssetData materialData =
                LoadMaterialData(scene->mMaterials[materialIndex], materialIndex, assetDirectory, sourcePath, importTextures);
            materialHandles.push_back(materialData.handle);
            AddMaterialDescription(result, std::move(materialData));
        }
    }

    void LoadMaterialTextures(
        const aiScene* scene,
        const std::filesystem::path& assetDirectory,
        const std::string& sourcePath,
        gns::assets::AssetLoadResult& result,
        std::unordered_set<gns::Handle>& loadedTextures)
    {
        if (scene == nullptr || !scene->HasMaterials())
        {
            return;
        }

        for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
        {
            const aiMaterial* material = scene->mMaterials[materialIndex];
            const std::optional<MaterialTextureSlot> textureSlot = FindFirstMaterialTextureSlot(material);
            if (!textureSlot)
            {
                continue;
            }

            std::optional<gns::assets::TextureAssetData> texture =
                LoadMaterialTextureData(scene, material, *textureSlot, assetDirectory, sourcePath);
            if (!texture)
            {
                LOG_WARNING("[AssetLoader]: Failed to load material texture.");
                continue;
            }

            AddTextureDescription(result, std::move(*texture), loadedTextures);
        }
    }

    gns::assets::MeshAssetData LoadMeshData(
        const aiMesh* mesh,
        uint32_t meshIndex,
        const std::string& sourcePath,
        const std::vector<gns::Handle>& materialHandles,
        const NodeTransform& transform)
    {
        gns::assets::MeshAssetData meshData;
        if (mesh == nullptr)
        {
            return meshData;
        }

        meshData.name = mesh->mName.C_Str();
        if (meshData.name.empty())
        {
            meshData.name = "Mesh_" + std::to_string(meshIndex);
        }

        meshData.handle = GetMeshArtifactHandle(sourcePath, meshIndex);
        if (mesh->mMaterialIndex < materialHandles.size())
        {
            meshData.materialHandle = materialHandles[mesh->mMaterialIndex];
        }

        meshData.position = transform.position;
        meshData.rotation = transform.rotation;
        meshData.scale = transform.scale;

        meshData.positions.reserve(mesh->mNumVertices);
        meshData.normals.reserve(mesh->mNumVertices);
        meshData.colors.reserve(mesh->mNumVertices);
        meshData.uvs.reserve(mesh->mNumVertices);
        for (size_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
        {
            meshData.positions.emplace_back(
                mesh->mVertices[vertexIndex].x,
                mesh->mVertices[vertexIndex].y,
                mesh->mVertices[vertexIndex].z);

            if (mesh->HasNormals())
            {
                meshData.normals.emplace_back(
                    mesh->mNormals[vertexIndex].x,
                    mesh->mNormals[vertexIndex].y,
                    mesh->mNormals[vertexIndex].z);
                meshData.colors.emplace_back(
                    mesh->mNormals[vertexIndex].x,
                    mesh->mNormals[vertexIndex].y,
                    mesh->mNormals[vertexIndex].z,
                    1.0f);
            }
            else
            {
                meshData.normals.emplace_back(0.0f, 1.0f, 0.0f);
                meshData.colors.emplace_back(1.0f);
            }

            if (mesh->HasTextureCoords(0))
            {
                meshData.uvs.emplace_back(
                    mesh->mTextureCoords[0][vertexIndex].x,
                    mesh->mTextureCoords[0][vertexIndex].y * -1.0f);
            }
            else
            {
                meshData.uvs.emplace_back(0.0f);
            }
        }

        if (mesh->HasTangentsAndBitangents())
        {
            meshData.tangents.reserve(mesh->mNumVertices);
            meshData.bitangents.reserve(mesh->mNumVertices);
            for (size_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
            {
                meshData.tangents.emplace_back(
                    mesh->mTangents[vertexIndex].x,
                    mesh->mTangents[vertexIndex].y,
                    mesh->mTangents[vertexIndex].z);
                meshData.bitangents.emplace_back(
                    mesh->mBitangents[vertexIndex].x,
                    mesh->mBitangents[vertexIndex].y,
                    mesh->mBitangents[vertexIndex].z);
            }
        }

        size_t indexCount = 0;
        for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
        {
            indexCount += mesh->mFaces[faceIndex].mNumIndices;
        }
        meshData.indices.reserve(indexCount);
        for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
        {
            const aiFace& face = mesh->mFaces[faceIndex];
            for (uint32_t index = 0; index < face.mNumIndices; ++index)
            {
                meshData.indices.push_back(face.mIndices[index]);
            }
        }

        return meshData;
    }

    void AddMeshDescription(
        gns::assets::AssetLoadResult& result,
        gns::assets::MeshAssetData meshData)
    {
        if (!meshData.handle.IsValid())
        {
            return;
        }

        const gns::Handle meshHandle = meshData.handle;
        result.assets.emplace_back(gns::assets::AssetDescription
        {
            .type = gns::assets::Mesh,
            .handle = meshHandle,
            .payload = std::move(meshData)
        });
    }

    void LoadMeshesFromNode(
        const aiScene* scene,
        const aiNode* node,
        const aiMatrix4x4& parentTransform,
        const std::string& sourcePath,
        const std::vector<gns::Handle>& materialHandles,
        bool flattenHierarchy,
        gns::assets::AssetLoadResult& result)
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

            AddMeshDescription(
                result,
                LoadMeshData(scene->mMeshes[meshIndex], meshIndex, sourcePath, materialHandles, transform));
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
                    result);
            }
        }
    }
}

gns::assets::AssetLoadResult gns::assets::AssetLoader::LoadSourceAsset(
    const std::filesystem::path& path)
{
    return LoadSourceAsset(path, AssetLoadOptions{});
}

gns::assets::AssetLoadResult gns::assets::AssetLoader::LoadSourceAsset(
    const std::filesystem::path& path,
    const AssetLoadOptions& loadOptions)
{
    AssetLoadResult result;
    result.sourcePath = path;
    result.loadOptions = loadOptions;

    if (IsTextureSourcePath(path))
    {
        std::unordered_set<gns::Handle> loadedTextures;
        std::optional<TextureAssetData> texture = DecodeTextureFile(path);
        if (!texture)
        {
            result.success = false;
            result.error = "Failed to decode texture file.";
            return result;
        }

        AddTextureDescription(result, std::move(*texture), loadedTextures);
        result.success = !result.assets.empty();
        if (!result.success)
        {
            result.error = "Texture file produced no texture data.";
        }
        return result;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path.string(),
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType);
    if (scene == nullptr)
    {
        result.success = false;
        result.error = importer.GetErrorString();
        LOG_ERROR(result.error);
        return result;
    }

    if (!scene->HasMeshes())
    {
        result.success = false;
        result.error = "Source asset does not contain meshes.";
        LOG_INFO("File: " + path.string() + " does not contain meshes.");
        return result;
    }

    const std::filesystem::path assetPath = gns::path::Normalize(path);
    const std::string sourcePath = ToProjectRelativeAssetString(assetPath);
    const std::filesystem::path assetDirectory = gns::path::ParentDirectory(assetPath);
    std::vector<gns::Handle> materialHandles;
    if (loadOptions.importMaterials && scene->HasMaterials())
    {
        LoadMaterials(
            scene,
            assetDirectory,
            sourcePath,
            loadOptions.importTextures,
            result,
            materialHandles);
    }

    std::unordered_set<gns::Handle> loadedTextures;
    if (loadOptions.importMaterials && loadOptions.importTextures)
    {
        LoadMaterialTextures(scene, assetDirectory, sourcePath, result, loadedTextures);
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
            result);
    }
    else
    {
        const NodeTransform identityTransform = {};
        for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
        {
            AddMeshDescription(
                result,
                LoadMeshData(scene->mMeshes[meshIndex], meshIndex, sourcePath, materialHandles, identityTransform));
        }
    }

    result.success = !result.assets.empty();
    if (!result.success)
    {
        result.error = "Source asset produced no mesh data.";
    }
    return result;
}
