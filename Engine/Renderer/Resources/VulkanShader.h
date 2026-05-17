#pragma once
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "VulkanResource.h"
#include "../../Object/IObject.h"
#include "../../Object/Material.h"


namespace gns
{
    struct Handle;
}

struct Shader;

namespace gns::rendering
{
    class Device;

    struct  VulkanShader : public VulkanResource
    {
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
        gns::MaterialLayout m_materialLayout;
        uint32_t m_globalDescriptorBindingMask = 0;
        VkShaderStageFlags m_drawPushConstantStageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        VkPipeline GetPipeline() { return m_pipeline; }
        VkPipelineLayout GetPipelineLayout() { return m_pipelineLayout; }
        VkDescriptorSetLayout GetDescriptorSetLayout() { return m_descriptorSetLayout; }
        VkDescriptorSetLayout GetDescriptorSetLayout(uint32_t setIndex) const
        {
            return setIndex < m_descriptorSetLayouts.size()
                ? m_descriptorSetLayouts[setIndex]
                : VK_NULL_HANDLE;
        }
        const gns::MaterialLayout& GetMaterialLayout() const { return m_materialLayout; }
        
        void Destroy() override;
    };
}
