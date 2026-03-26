#pragma once
#include <vulkan/vulkan_core.h>

#include "../../Object/IObject.h"

namespace gns
{
    struct Handle;
}

struct Shader;

namespace gns::rendering
{
    class VulkanShader : public Object
    {
        VkPipeline m_pipeline;
        VkPipelineLayout m_pipelineLayout;
        VkDescriptorSetLayout m_descriptorSetLayout;
    public:
        void CreatePipeline(Shader shader);
    };
}
