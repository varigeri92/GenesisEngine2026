#include "gnspch.h"
#include "VulkanImage.h"

#include "vkutils.h"
#include "vulkan_log.h"
#include "Device.h"

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

void gns::rendering::VulkanImage::CreateImage(VkExtent3D size, VkFormat format,
    VkImageUsageFlags usage, bool mipmapped)
{
    GNS_PROFILE_FUNCTION();
    imageFormat = format;
    imageExtent = size;
    
    VkDevice device = m_device->GetDevice();
    VmaAllocator allocator = m_device->GetAlocator();
    
    VkImageCreateInfo img_info = utils::ImageCreateInfo(format, usage, size);
    if (mipmapped) {
        img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
    }

    // always allocate images on dedicated GPU memory
    VmaAllocationCreateInfo allocinfo = {};
    allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocinfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // allocate and create the image
    VK_CHECK(vmaCreateImage(allocator, &img_info, &allocinfo, &image, &allocation, nullptr));

    // if the format is a depth format, we will need to have it use the correct
    // aspect flag
    VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D32_SFLOAT) {
        aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    // build an image-view for the image
    VkImageViewCreateInfo view_info = utils::ImageViewCreateInfo(format, image, aspectFlag);
    view_info.subresourceRange.levelCount = img_info.mipLevels;

    VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &imageView));
}

void gns::rendering::VulkanImage::CreateImage(const void* data, VkExtent3D size, VkFormat format,
    VkImageUsageFlags usage, bool mipmapped)
{
    GNS_PROFILE_FUNCTION();
    size_t data_size = 
        static_cast<size_t>(size.depth) * static_cast<size_t>(size.width) * static_cast<size_t>(size.height) * 4;
    VulkanBuffer uploadBuffer;
    uploadBuffer.allocator = m_device->GetAlocator();
    uploadBuffer.CreateBuffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    memcpy(uploadBuffer.info.pMappedData, data, data_size);

    CreateImage(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

    m_device->ImmediateSubmit([&](VkCommandBuffer cmd) {
        utils::TransitionImage(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = size;

        // copy the buffer into the image
        vkCmdCopyBufferToImage(cmd, uploadBuffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
            &copyRegion);

        utils::TransitionImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });
    uploadBuffer.reset();
}

void gns::rendering::VulkanImage::DestroyImage()
{
    GNS_PROFILE_FUNCTION();
    if (m_device == nullptr)
    {
        return;
    }

    if (imageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_device->GetDevice(), imageView, nullptr);
        imageView = VK_NULL_HANDLE;
    }

    if (image != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
    {
        vmaDestroyImage(m_device->GetAlocator(), image, allocation);
        image = VK_NULL_HANDLE;
        allocation = VK_NULL_HANDLE;
    }
}

void gns::rendering::VulkanImage::Destroy()
{
    GNS_PROFILE_FUNCTION();
    DestroyImage();
}
