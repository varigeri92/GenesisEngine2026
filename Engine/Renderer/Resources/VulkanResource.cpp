#include "gnspch.h"
#include "VulkanResource.h"

std::unordered_map<gns::Handle, gns::VulkanResource*> gns::VulkanResource::resourceMap = {};

void gns::VulkanResource::FreeAll()
{
    for (const auto& [handle, resource] : gns::VulkanResource::resourceMap)
    {
        resource->Destroy();
    }
    resourceMap.clear();
}
