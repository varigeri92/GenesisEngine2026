#pragma once
#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

struct VulkanBuffer
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo info{};

    VmaAllocator allocator = VK_NULL_HANDLE;
    size_t size = 0;
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    VulkanBuffer() = default;

    VulkanBuffer(VmaAllocator alloc,
                 VkBuffer buf,
                 VmaAllocation allocHandle,
                 const VmaAllocationInfo& allocInfo = {})
        : buffer(buf),
          allocation(allocHandle),
          info(allocInfo),
          allocator(alloc)
    {
    }

    ~VulkanBuffer()
    {
        reset();
    }

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    VulkanBuffer(VulkanBuffer&& other) noexcept
        : buffer(other.buffer),
          allocation(other.allocation),
          info(other.info),
          allocator(other.allocator),
          size(other.size),
          usage(other.usage),
          memoryUsage(other.memoryUsage)
    {
        other.buffer = VK_NULL_HANDLE;
        other.allocation = VK_NULL_HANDLE;
        other.info = {};
        other.allocator = VK_NULL_HANDLE;
        other.size = 0;
    }

    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept
    {
        if (this != &other)
        {
            reset();

            buffer = other.buffer;
            allocation = other.allocation;
            info = other.info;
            allocator = other.allocator;
            size = other.size;
            usage = other.usage;
            memoryUsage = other.memoryUsage;

            other.buffer = VK_NULL_HANDLE;
            other.allocation = VK_NULL_HANDLE;
            other.info = {};
            other.allocator = VK_NULL_HANDLE;
            other.size = 0;
        }
        return *this;
    }

    void reset()
    {
        if (allocator != VK_NULL_HANDLE && buffer != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(allocator, buffer, allocation);
        }

        buffer = VK_NULL_HANDLE;
        allocation = VK_NULL_HANDLE;
        info = {};
        allocator = VK_NULL_HANDLE;
        size = 0;
    }

    explicit operator bool() const noexcept
    {
        return buffer != VK_NULL_HANDLE;
    }
    
    void CreateBuffer(size_t allocSize, VkBufferUsageFlags bufferUsageFlags, VmaMemoryUsage memoryUsage);
};
