#pragma once
#include "VulkanShader.h"
#include "../Vulkan/VulkanBuffer.h"

namespace gns::rendering
{
    struct VulkanMaterial : public gns::VulkanResource
    {
        VulkanShader* vkShader;
        VulkanBuffer* buffer;
        VkDescriptorSet descriptorSet;
    };
}
