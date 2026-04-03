#include "gnspch.h"
#include "VulkanTexture.h"
#include "../Vulkan/vulkan_log.h"
#include "../Vulkan/Device.h"
#include "../Vulkan/vkutils.h"

void gns::rendering::VulkanTexture::Createtexture(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, 
                                                  bool mipmapped)
{
    image.CreateImage(size, format, usage, mipmapped);
}

void gns::rendering::VulkanTexture::Createtexture(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage,
    bool mipmapped)
{
    image.CreateImage(data, size, format, usage, mipmapped);
}

void gns::rendering::VulkanTexture::DestroyTexture(VkDevice device, VmaAllocator allocator)
{
    vkDestroyImageView(device, image.imageView, nullptr);
    vkDestroyImage(device, image.image, nullptr);
}
