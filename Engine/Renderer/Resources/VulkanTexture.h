#pragma once
#include "VulkanResource.h"
#include "../Vulkan/VulkanImage.h"

namespace gns::rendering
{
    struct VulkanTexture : public VulkanResource
    {
        VulkanImage image;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;

        void CreateTexture(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
        void CreateTexture(const void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
        void Destroy() override;
    };
}
