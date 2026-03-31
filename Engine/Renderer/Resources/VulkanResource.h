#pragma once
#include "../../Core/Handles.h"

namespace gns
{
    struct VulkanResource;
    template <typename Resource_T> 
    concept DerivedFromVulkanResource = std::derived_from<Resource_T, gns::VulkanResource>;
    
    struct VulkanResource
    {
    private:
        static std::unordered_map<gns::Handle, VulkanResource*> resourceMap;
        Handle resourceHandle;
    public:
        Handle GetHandle() const {return resourceHandle;}
        
        template <DerivedFromVulkanResource Resource_T, typename... Args>
        static Resource_T* Create(Args&& ... args)
        {

            Resource_T* res = new Resource_T(std::forward<Args>(args)...);
            res->resourceHandle = Handle::New();
            LOG_INFO("[VulkanResource]: Created a new resource handle!");
            LOG_INFO(std::to_string(res->resourceHandle.Get()));
            auto [it, inserted] = resourceMap.try_emplace(res->resourceHandle, res);
            if (inserted)
                return res;
            
            LOG_ERROR("[VulkanResource]: Cannot Create Vulkan Resource!");
            delete res;
            return nullptr;
        }
        
        template <DerivedFromVulkanResource Resource_T>
        static Resource_T* Get(const gns::Handle handle)
        {
            if (resourceMap.contains(handle))
            {
                return reinterpret_cast<Resource_T*>(resourceMap.at(handle));
            }
            LOG_WARNING("[VulkanResource]: Does not contain the key!");
            LOG_WARNING(std::to_string(handle.Get()));
            return nullptr;
        }
    };
}
