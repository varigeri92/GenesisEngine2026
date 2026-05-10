#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>
#include "VulkanResource.h"
#include "../../Object/IObject.h"


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
        VkPipeline GetPipeline() { return m_pipeline; }
        VkPipelineLayout GetPipelineLayout() { return m_pipelineLayout; }
        VkDescriptorSetLayout GetDescriptorSetLayout() { return m_descriptorSetLayout; }
        VkDescriptorSetLayout GetDescriptorSetLayout(uint32_t setIndex) const
        {
            return setIndex < m_descriptorSetLayouts.size()
                ? m_descriptorSetLayouts[setIndex]
                : VK_NULL_HANDLE;
        }
        
        void Destroy() override;
    };
}
