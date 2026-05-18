#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vector>
#include "../Resources/VulkanShader.h"
#include "VulkanBuffer.h"

namespace gns
{
    struct Material;
    struct Mesh;
    struct Shader;
    struct Texture;

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

    struct PendingMeshUpload
    {
        Handle meshHandle;
        Mesh* mesh = nullptr;
    };

    struct PendingTextureUpload
    {
        Handle textureHandle;
        Texture* texture = nullptr;
    };

    struct PendingShaderUpload
    {
        Handle shaderHandle;
        Shader* shader = nullptr;
    };

    struct PendingMaterialUpload
    {
        Handle materialHandle;
        Material* material = nullptr;
    };

    struct RenderUploadQueue
    {
        std::vector<PendingMeshUpload> meshUploads;
        std::vector<PendingTextureUpload> textureUploads;
        std::vector<PendingShaderUpload> shaderUploads;
        std::vector<PendingMaterialUpload> materialUploads;

        bool IsEmpty() const
        {
            return meshUploads.empty() &&
                textureUploads.empty() &&
                shaderUploads.empty() &&
                materialUploads.empty();
        }

        void Clear()
        {
            meshUploads.clear();
            textureUploads.clear();
            shaderUploads.clear();
            materialUploads.clear();
        }
    };

    struct RenderFramePacket
    {
        std::vector<DrawData> drawData;
        GlobalFrameDataDescriptor globalFrameData;
        bool hasGlobalFrameData = false;

        void Clear()
        {
            drawData.clear();
            globalFrameData = {};
            hasGlobalFrameData = false;
        }
    };

    struct RenderSubmission
    {
        RenderFramePacket packet;
        RenderUploadQueue uploads;

        void Clear()
        {
            packet.Clear();
            uploads.Clear();
        }
    };
}
