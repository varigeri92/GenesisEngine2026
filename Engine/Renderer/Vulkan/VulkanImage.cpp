#include "gnspch.h"
#include "VulkanImage.h"

gns::rendering::VulkanImage::VulkanImage() : 
    image(VK_NULL_HANDLE), 
    imageView(VK_NULL_HANDLE), 
    allocation(VK_NULL_HANDLE), 
    imageExtent{0,0,1}, 
    imageFormat(VK_FORMAT_UNDEFINED)
{}

gns::rendering::VulkanImage::VulkanImage(VkExtent3D imageExtent, VkFormat imageFormat/* = VK_FORMAT_R8G8B8_UNORM */) :
    image(VK_NULL_HANDLE), 
    imageView(VK_NULL_HANDLE), 
    allocation(VK_NULL_HANDLE), 
    imageExtent(imageExtent), 
    imageFormat(imageFormat)
{
}
