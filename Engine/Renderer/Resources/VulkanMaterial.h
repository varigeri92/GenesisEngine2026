#pragma once
#include <vector>

#include "VulkanShader.h"
#include "../Vulkan/VulkanBuffer.h"

namespace gns::rendering
{
    struct VulkanMaterialTextureBinding
    {
        uint32_t binding = gns::InvalidMaterialBinding;
        Handle textureHandle;
        VkImageView imageView = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
    };

    struct VulkanMaterial : public gns::VulkanResource
    {
        VulkanShader* vkShader = nullptr;
        VulkanBuffer materialDataBuffer;
        VkDescriptorSet materialDataSet = VK_NULL_HANDLE;
        VkDescriptorSet textureSet = VK_NULL_HANDLE;
        VkDescriptorSetLayout materialDataLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout textureLayout = VK_NULL_HANDLE;
        uint32_t materialDataSetIndex = gns::InvalidMaterialBinding;
        uint32_t materialDataBinding = gns::InvalidMaterialBinding;
        MaterialDescriptorKind materialDataDescriptorKind = MaterialDescriptorKind::None;
        uint32_t textureSetIndex = gns::InvalidMaterialBinding;
        std::vector<uint8_t> materialDataCache;
        std::vector<VulkanMaterialTextureBinding> textureBindings;

        void Destroy() override;
    };
}
