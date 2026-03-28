#pragma once
#include <span>
#include <glm/glm.hpp>
#include "VulkanBuffer.h"


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

struct VulkanMesh
{
    VulkanBuffer indexBuffer;
    VulkanBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;
    
    static VulkanMesh UploadMesh(gns::rendering::Device& device, VmaAllocator allocator, std::span<uint32_t> indices, std::span<Vertex> vertices);
};

struct GPUDrawPushConstants {
    glm::mat4 worldMatrix;
    VkDeviceAddress vertexBuffer;
};