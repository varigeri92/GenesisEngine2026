#pragma once
#include "VulkanResource.h"
#include "../Vulkan/VulkanImage.h"

namespace gns::rendering
{
    struct VulkanTexture : public VulkanResource
    {
        VulkanImage image;
        VkDescriptorSet descriptorSet;
        VkSampler sampler;
        void Createtexture(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
        void Createtexture(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
        void DestroyTexture(VkDevice device, VmaAllocator allocator);
    };
}
