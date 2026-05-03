#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "../Core/Handles.h"
#include "../Object/IObject.h"

namespace gns::assets
{
    enum AssetType { Generic, Mesh, Texture, Shader, ComputeShader, Material };

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
    };
}
