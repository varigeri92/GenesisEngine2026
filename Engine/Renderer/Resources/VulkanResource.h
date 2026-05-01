#pragma once
#include <concepts>
#include <memory>
#include <unordered_map>
#include <utility>

#include "../../Core/Handles.h"

namespace gns
{
    namespace rendering
    {
        class Device;
    }

    struct VulkanResource;
    template <typename Resource_T> 
    concept DerivedFromVulkanResource = std::derived_from<Resource_T, gns::VulkanResource>;
    
    struct VulkanResource
    {
        friend class rendering::Device;
        friend struct VulkanResourceRegistry;
    private:
        Handle resourceHandle;
    protected:
        rendering::Device* m_device;
    public:
        virtual ~VulkanResource() = default;
        Handle GetHandle() const {return resourceHandle;}
        virtual void Destroy() = 0;
    };

    struct VulkanResourceRegistry
    {
    private:
        std::unordered_map<gns::Handle, std::unique_ptr<VulkanResource>> resourceMap;
    public:
        template <DerivedFromVulkanResource Resource_T, typename... Args>
        Resource_T* Create(rendering::Device* device, Args&& ... args)
        {
            auto res = std::make_unique<Resource_T>(std::forward<Args>(args)...);
            res->m_device = device;
            res->resourceHandle = Handle::New();
            LOG_INFO("[VulkanResourceRegistry]: Created a new resource handle!");
            LOG_INFO(std::to_string(res->resourceHandle.Get()));
            Resource_T* resourcePtr = res.get();
            auto [it, inserted] = resourceMap.try_emplace(res->resourceHandle, std::move(res));
            if (inserted)
                return resourcePtr;
            
            LOG_ERROR("[VulkanResourceRegistry]: Cannot create Vulkan resource!");
            return nullptr;
        }
        
        template <DerivedFromVulkanResource Resource_T>
        Resource_T* Get(const gns::Handle handle) const
        {
            if (const auto it = resourceMap.find(handle); it != resourceMap.end())
            {
                Resource_T* resource = dynamic_cast<Resource_T*>(it->second.get());
                if (resource != nullptr)
                {
                    return resource;
                }

                LOG_ERROR("[VulkanResourceRegistry]: Resource type mismatch!");
                LOG_ERROR(std::to_string(handle.Get()));
                return nullptr;
            }
            LOG_WARNING("[VulkanResourceRegistry]: Does not contain the key!");
            LOG_WARNING(std::to_string(handle.Get()));
            return nullptr;
        }
        
        void DestroyAll();
    };
}
