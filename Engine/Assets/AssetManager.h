#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "../Core/Handles.h"
#include "../Object/IObject.h"

namespace gns
{
    struct Material;
    struct Mesh;
    struct Texture;
}

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
            return reinterpret_cast<Object_T*>(object);
        }
    };
    
    class AssetManager
    {
    public:
        static std::vector<LoadedObject> LoadAsset(const std::string& path);
        static std::vector<LoadedObject> LoadAsset(const std::string& path, const AssetLoadOptions& loadOptions);
        GNS_API static gns::Mesh* EnsureMeshLoaded(Handle meshHandle);
        GNS_API static gns::Material* EnsureMaterialLoaded(Handle materialHandle);
        GNS_API static gns::Texture* EnsureTextureLoaded(Handle textureHandle);
        GNS_API static bool ApplyImportedMaterialDefaults(gns::Material& material);
        GNS_API static Handle GetModelAssetHandle(const std::filesystem::path& sourcePath);
        GNS_API static Handle GetMeshArtifactHandle(const std::filesystem::path& sourcePath, uint32_t meshIndex);
        GNS_API static Handle GetMaterialArtifactHandle(const std::filesystem::path& sourcePath, uint32_t materialIndex);
        GNS_API static Handle GetTextureArtifactHandle(const std::filesystem::path& texturePath);
    };
}
