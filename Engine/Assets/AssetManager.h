#pragma once
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
    
    struct LoadedObject
    {
        Handle objectHandle;
        Object* object;
        Handle materialHandle;
        
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
    };
}
