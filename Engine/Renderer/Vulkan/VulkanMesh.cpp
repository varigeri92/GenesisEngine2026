#include "gnspch.h"
#include "VulkanMesh.h"
#include "vma/vk_mem_alloc.h"
#include <span>
#include "Device.h"

VulkanMesh::~VulkanMesh()
{
    Destroy();
}

VulkanMesh& VulkanMesh::UploadMesh(gns::rendering::Device& device, VmaAllocator allocator, std::span<uint32_t> indices,
                                   std::span<Vertex> vertices)
{
    const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    VulkanMesh& newSurface = *VulkanResource::Create<VulkanMesh>(&device);
    newSurface.vertexBuffer.allocator = allocator;
    newSurface.vertexBuffer.CreateBuffer(
        vertexBufferSize, 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    //find the adress of the vertex buffer
    VkBufferDeviceAddressInfo deviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newSurface.vertexBuffer.buffer };
    newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(device.GetDevice(), &deviceAdressInfo);

    newSurface.indexBuffer.allocator = allocator;
    //create index buffer
    newSurface.indexBuffer.CreateBuffer(
        indexBufferSize, 
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    {
        VulkanBuffer staging;
        staging.allocator = allocator;
        staging.CreateBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

        void* data = device.GetMappedDataFromAllocation(staging.allocation);

        // copy vertex buffer
        memcpy(data, vertices.data(), vertexBufferSize);
        // copy index buffer
        memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

        device.ImmediateSubmit([&](VkCommandBuffer cmd) {
            VkBufferCopy vertexCopy{ 0 };
            vertexCopy.dstOffset = 0;
            vertexCopy.srcOffset = 0;
            vertexCopy.size = vertexBufferSize;

            vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

            VkBufferCopy indexCopy{ 0 };
            indexCopy.dstOffset = 0;
            indexCopy.srcOffset = vertexBufferSize;
            indexCopy.size = indexBufferSize;

            vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
        });
    }
    return newSurface;
}

void VulkanMesh::Destroy()
{
    m_device->DestroyMesh(*this);
}
