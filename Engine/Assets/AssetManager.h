#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "../Core/Handles.h"
#include "../Object/IObject.h"

namespace gns::assets
{
    enum AssetType { Generic, Mesh, Texture, Shader, Material };

    struct Asset
    {
        gns::Handle assetHandle;
        std::string assetName;
        std::string assetPath;
        AssetType assetType;
    };

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
        static std::unordered_map<gns::Handle, Asset> AssetMap;
    public:
        static std::vector<LoadedObject> LoadAsset(const std::string& path);
        static std::vector<LoadedObject> LoadAsset(const std::string& path, const AssetLoadOptions& loadOptions);
    };
}
