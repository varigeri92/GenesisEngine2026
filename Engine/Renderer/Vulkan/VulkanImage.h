#pragma once
#include <vulkan\vulkan.h>
#include <vma/vk_mem_alloc.h>

#include "../Resources/VulkanResource.h"

namespace gns::rendering
{
    class Device;

    class VulkanImage : public VulkanResource
    {
    public:
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkExtent3D imageExtent  = {};
        VkFormat imageFormat = VK_FORMAT_R8G8B8_UNORM;
        
        
        VulkanImage();
        VulkanImage(VkExtent3D imageExtent, VkFormat imageFormat = VK_FORMAT_R8G8B8_UNORM);
        
        void CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
        void CreateImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
        void Destroy() override;
        private:
        void DestroyImage() const;
    };
}
