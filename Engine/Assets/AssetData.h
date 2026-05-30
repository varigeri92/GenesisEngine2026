#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

#include <glm/glm.hpp>

#include "../Core/Handles.h"
#include "../Object/IObject.h"

namespace gns::assets
{
    enum AssetType { Generic, Mesh, Texture, Shader, ComputeShader, Material };
    enum class AssetArtifactType { Unknown, Mesh, Material, Texture };

    struct AssetLoadOptions
    {
        bool flattenHierarchy = false;
        bool importSkeleton = true;
        bool importMaterials = true;
        bool importTextures = true;
    };

    enum class AssetTextureFormat
    {
        Unknown,
        R8G8B8A8_UNorm,
        R8G8B8_UNorm
    };

    struct MeshAssetData
    {
        Handle handle;
        Handle materialHandle;
        std::string name;
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
        std::vector<uint32_t> indices;
        std::vector<glm::vec3> positions;
        std::vector<glm::vec4> colors;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec3> tangents;
        std::vector<glm::vec3> bitangents;
        std::vector<glm::vec2> uvs;
    };

    struct TextureAssetData
    {
        Handle handle;
        std::string name;
        std::filesystem::path assetPath;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channels = 0;
        AssetTextureFormat format = AssetTextureFormat::Unknown;
        std::vector<uint8_t> pixels;
    };

    struct MaterialTextureReference
    {
        std::string propertyName;
        Handle textureHandle;
    };

    struct MaterialAssetData
    {
        Handle handle;
        std::string name;
        std::vector<MaterialTextureReference> textures;
    };

    using AssetPayload = std::variant<
        std::monostate,
        MeshAssetData,
        TextureAssetData,
        MaterialAssetData>;

    struct AssetDescription
    {
        AssetType type = Generic;
        Handle handle;
        AssetPayload payload;
    };

    struct AssetLoadResult
    {
        std::filesystem::path sourcePath;
        AssetLoadOptions loadOptions;
        std::vector<AssetDescription> assets;
        bool success = false;
        std::string error;
    };

    struct AssetSourceReference
    {
        std::filesystem::path sourcePath;
        AssetLoadOptions loadOptions;
        bool valid = false;
    };

    struct LoadedObject
    {
        Handle objectHandle;
        Object* object;
        Handle materialHandle;
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);

        template <DerivedFromObject Object_T>
        Object_T* As()
        {
            return dynamic_cast<Object_T*>(object);
        }

        template <DerivedFromObject Object_T>
        Object_T* As() const
        {
            return dynamic_cast<Object_T*>(object);
        }
    };
}
