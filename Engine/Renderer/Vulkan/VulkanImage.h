#pragma once
#include <vulkan\vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace gns::rendering
{
    class Device;

    class VulkanImage
    {
    public:
        VkImage image;
        VkImageView imageView;
        VmaAllocation allocation;
        VkExtent3D imageExtent;
        VkFormat imageFormat;
        
        
        VulkanImage();
        VulkanImage(VkExtent3D imageExtent, VkFormat imageFormat = VK_FORMAT_R8G8B8_UNORM);
        
        void CreateImage(Device device);
    };
}