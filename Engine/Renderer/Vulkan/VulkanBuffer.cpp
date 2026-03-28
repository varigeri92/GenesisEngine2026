#include "gnspch.h"
#include "VulkanBuffer.h"

#include "vulkan_log.h"

void VulkanBuffer::CreateBuffer(size_t allocSize, VkBufferUsageFlags bufferUsageFlags, VmaMemoryUsage memoryUsage)
{
    // allocate buffer
    VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.pNext = nullptr;
    bufferInfo.size = allocSize;

    bufferInfo.usage = bufferUsageFlags;

    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage = memoryUsage;
    vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    // allocate the buffer
    VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &buffer, &allocation, &info));

    
}
