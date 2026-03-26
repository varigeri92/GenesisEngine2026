#pragma once
#include <vector>
#include <vulkan/vulkan.h>

#include "../Shader.h"

namespace gns::rendering
{
	class Device;

	class PipelineBuilder
	{
	private:
		VkShaderModule vertexModule, fragmentModule;
		Device* m_device;
	public:
	    PipelineBuilder(Device* device);
		~PipelineBuilder();
		enum BlendingMode
		{
			disabled, additive, alpha
		};

	    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
	    VkPipelineInputAssemblyStateCreateInfo m_inputAssembly;
	    VkPipelineRasterizationStateCreateInfo m_rasterizer;
	    VkPipelineColorBlendAttachmentState m_colorBlendAttachment;
	    VkPipelineMultisampleStateCreateInfo m_multisampling;
	    VkPipelineLayout m_pipelineLayout;
	    VkPipelineDepthStencilStateCreateInfo m_depthStencil;
	    VkPipelineRenderingCreateInfo m_renderInfo;
	    VkFormat m_colorAttachmentFormat;


	    void Clear();

	    VkPipeline BuildPipeline(VkDevice device);

	    void SetShaders(Shader& shader);
	    void SetInputTopology(VkPrimitiveTopology topology);
	    void SetPolygonMode(VkPolygonMode mode);
	    void SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
	    void SetMultisampling(bool enable = false);
	    void SetBlending(BlendingMode = disabled);
	    void DisableBlending();
	    void EnableBlendingAdditive();
	    void EnableBlendingAlphaBlend();

	    void SetColorAttachmentFormat(VkFormat format);
	    void SetDepthFormat(VkFormat format);
	    void DisableDepthTest();
	    void EnableDepthTest(bool depthWriteEnable, VkCompareOp op);
	private:
	    void SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
	};
}
