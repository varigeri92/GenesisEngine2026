#include "gnspch.h"
#include "VulkanResource.h"

void gns::VulkanResourceRegistry::DestroyAll()
{
    for (const auto& [handle, resource] : resourceMap)
    {
        resource->Destroy();
    }
    resourceMap.clear();
}
