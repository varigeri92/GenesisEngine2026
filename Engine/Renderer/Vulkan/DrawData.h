#pragma once
#include <glm/glm.hpp>
#include "../Resources/VulkanShader.h"
#include "VulkanBuffer.h"

namespace gns
{
    namespace rendering
    {
        struct VulkanMaterial;
        struct VulkanTexture;
    }

    struct GpuDataDescriptor
    {
        const void* data = nullptr;
        size_t size = 0;

        bool IsValid() const { return data != nullptr && size > 0; }
        
        template<typename T>
        static GpuDataDescriptor GetFromType(T* data)
        {
            return {.data = static_cast<const void*>(data), .size = sizeof(T)};
        }

        static GpuDataDescriptor GetFromMemory(const void* data, size_t size)
        {
            return {.data = data, .size = size};
        }
    };

    struct MaterialTextureBinding
    {
        uint32_t binding = InvalidMaterialBinding;
        gns::rendering::VulkanTexture* texture = nullptr;
    };

    struct GlobalFrameDataDescriptor
    {
        GpuDataDescriptor sceneData;
        GpuDataDescriptor directionalLights;
        GpuDataDescriptor pointLights;
        GpuDataDescriptor spotLights;

        bool IsValid() const { return sceneData.IsValid(); }
    };

    struct DrawData
    {
        glm::mat4 transform;
        VkBuffer vk_indexBuffer;
        gns::rendering::VulkanShader* vkShader;
        gns::rendering::VulkanMaterial* vkMaterial = nullptr;
        VkDeviceAddress vk_vertexBufferAddress;
        size_t StartIndex;
        size_t Count;
        uint32_t index;
    };
}
