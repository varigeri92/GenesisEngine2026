#include "gnspch.h"
#include "VulkanTexture.h"

#include "../Vulkan/Device.h"

void gns::rendering::VulkanTexture::CreateTexture(
    VkExtent3D size,
    VkFormat format,
    VkImageUsageFlags usage,
    bool mipmapped)
{
    image.m_device = m_device;
    image.CreateImage(size, format, usage, mipmapped);
}

void gns::rendering::VulkanTexture::CreateTexture(
    const void* data,
    VkExtent3D size,
    VkFormat format,
    VkImageUsageFlags usage,
    bool mipmapped)
{
    image.m_device = m_device;
    image.CreateImage(data, size, format, usage, mipmapped);
}

void gns::rendering::VulkanTexture::Destroy()
{
    image.Destroy();

    if (m_device != nullptr && sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(m_device->GetDevice(), sampler, nullptr);
        sampler = VK_NULL_HANDLE;
    }

    descriptorSet = VK_NULL_HANDLE;
}
