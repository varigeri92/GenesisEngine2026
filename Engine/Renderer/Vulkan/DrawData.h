#pragma once
#include <glm/glm.hpp>
#include "../Resources/VulkanShader.h"
#include "VulkanBuffer.h"

namespace gns
{
    struct DrawData
    {
        glm::mat4 transform;
        VkBuffer vk_indexBuffer;
        gns::rendering::VulkanShader* vkShader; 
        uint64_t albedoTextureDescriptor;
        VkDeviceAddress vk_vertexBufferAddress;
        size_t StartIndex;
        size_t Count;
    };
    
    struct GpuDataDescriptor
    {
        void* data;
        size_t size;
        
        template<typename T>
        static GpuDataDescriptor GetFromType(T* data)
        {
            return {.data = static_cast<void*>(data), .size = sizeof(T)};
        }
    };
}
