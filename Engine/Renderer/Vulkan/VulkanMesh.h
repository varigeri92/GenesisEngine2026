#pragma once
#include <span>
#include <glm/glm.hpp>
#include "VulkanBuffer.h"
#include "../Resources/VulkanResource.h"


namespace gns::rendering
{
    class Device;
}

struct Vertex {
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;
};

struct VulkanMesh : public gns::VulkanResource
{
    VulkanBuffer indexBuffer;
    VulkanBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;
    uint32_t startIndex;
    uint32_t count;
    static VulkanMesh& UploadMesh(
        gns::rendering::Device& device, VmaAllocator allocator, std::span<uint32_t> indices, std::span<Vertex> vertices);
    void Destroy() override;
};

struct GPUDrawPushConstants {
    glm::mat4 worldMatrix;
    VkDeviceAddress vertexBuffer;
};
