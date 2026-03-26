#include "gnspch.h"
#include "PipelineBuilder.h"
#include "Device.h"
#include "vkutils.h"
#include "../Utils/FileSystemUtils.h"
#include "../Utils/Path.h"

namespace gns::rendering
{
	PipelineBuilder::PipelineBuilder(Device* device):m_device(device)
	{ Clear(); }

	PipelineBuilder::~PipelineBuilder()
	{
		vkDestroyShaderModule(m_device->GetDevice(), vertexModule, nullptr);
		vkDestroyShaderModule(m_device->GetDevice(), fragmentModule, nullptr);
	}

	void PipelineBuilder::Clear()
	{
	    m_inputAssembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	    m_rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	    m_colorBlendAttachment = {};
	    m_multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	    m_pipelineLayout = {};
	    m_depthStencil = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	    m_renderInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	    m_shaderStages.clear();
	}

	VkPipeline PipelineBuilder::BuildPipeline(VkDevice device)
	{
	    VkPipelineViewportStateCreateInfo viewportState = {};
	    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	    viewportState.pNext = nullptr;

	    viewportState.viewportCount = 1;
	    viewportState.scissorCount = 1;

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
	    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	    colorBlending.pNext = nullptr;
	    colorBlending.logicOpEnable = VK_FALSE;
	    colorBlending.logicOp = VK_LOGIC_OP_COPY;
	    colorBlending.attachmentCount = 1;
	    colorBlending.pAttachments = &m_colorBlendAttachment;

	    VkPipelineVertexInputStateCreateInfo _vertexInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

	    VkGraphicsPipelineCreateInfo pipelineInfo = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	    pipelineInfo.pNext = &m_renderInfo;
	    pipelineInfo.stageCount = (uint32_t)m_shaderStages.size();
	    pipelineInfo.pStages = m_shaderStages.data();
	    pipelineInfo.pVertexInputState = &_vertexInputInfo;
	    pipelineInfo.pInputAssemblyState = &m_inputAssembly;
	    pipelineInfo.pViewportState = &viewportState;
	    pipelineInfo.pRasterizationState = &m_rasterizer;
	    pipelineInfo.pMultisampleState = &m_multisampling;
	    pipelineInfo.pColorBlendState = &colorBlending;
	    pipelineInfo.pDepthStencilState = &m_depthStencil;
	    pipelineInfo.layout = m_pipelineLayout;

	    VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	    VkPipelineDynamicStateCreateInfo dynamicInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	    dynamicInfo.pDynamicStates = &state[0];
	    dynamicInfo.dynamicStateCount = 2;

	    pipelineInfo.pDynamicState = &dynamicInfo;

	    VkPipeline newPipeline;
	    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
	        nullptr, &newPipeline)
	        != VK_SUCCESS) {
	        LOG_ERROR("Failed To Create Pipeline");
	        return VK_NULL_HANDLE;
	    }
	    else {
	        return newPipeline;
	    }
	}

	void PipelineBuilder::SetShaders(Shader& shader)
	{
		std::string fragmentShaderPath = shader.GetFragmentShaderPath();
		std::string vertexShaderPath = shader.GetVertexShaderPath();
		if (!fileUtils::HasFileExtension(fragmentShaderPath, "spv"))
			fragmentShaderPath += ".spv";

		LOG_INFO(fragmentShaderPath);
		gns::path::InResourcesDirectory(fragmentShaderPath);
		if (!utils::LoadShaderModule(
			gns::path::InResourcesDirectory(fragmentShaderPath).string(), m_device->GetDevice(), &fragmentModule)) {
			LOG_ERROR("Can't build fragment shader module");
		}
		
		if (!fileUtils::HasFileExtension(vertexShaderPath, "spv"))
			vertexShaderPath += ".spv";

		LOG_INFO(vertexShaderPath);
		if (!utils::LoadShaderModule(
			gns::path::InResourcesDirectory(vertexShaderPath).string(), m_device->GetDevice(), &vertexModule)) {
			LOG_ERROR("Can't build vertex shader module");
		}
		SetShaders(vertexModule, fragmentModule);
	}

	void PipelineBuilder::SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader)
	{
	    m_shaderStages.clear();

	    m_shaderStages.push_back(
	        utils::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));

	    m_shaderStages.push_back(
	        utils::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
	}


	void PipelineBuilder::SetInputTopology(VkPrimitiveTopology topology)
	{
	    m_inputAssembly.topology = topology;
	    m_inputAssembly.primitiveRestartEnable = VK_FALSE;
	}

	void PipelineBuilder::SetPolygonMode(VkPolygonMode mode)
	{
		m_rasterizer.polygonMode = mode;
		m_rasterizer.lineWidth = 1.f;
	}

	void PipelineBuilder::SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
	{
		m_rasterizer.cullMode = cullMode;
		m_rasterizer.frontFace = frontFace;
	}

	void PipelineBuilder::SetMultisampling(bool enable)
	{
		m_multisampling.sampleShadingEnable = enable? VK_TRUE : VK_FALSE;
		m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		m_multisampling.minSampleShading = 1.0f;
		m_multisampling.pSampleMask = nullptr;
		m_multisampling.alphaToCoverageEnable = VK_FALSE;
		m_multisampling.alphaToOneEnable = VK_FALSE;
	}

	void PipelineBuilder::SetBlending(BlendingMode blendingMode)
	{
		switch (blendingMode) {
		case disabled:
			DisableBlending();
			break;
		case additive:
			EnableBlendingAdditive();
			break;
		case alpha:
			EnableBlendingAlphaBlend();
			break;
		default: ;
		}
	}

	void PipelineBuilder::DisableBlending()
	{
		m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		m_colorBlendAttachment.blendEnable = VK_FALSE;
	}

	void PipelineBuilder::EnableBlendingAdditive()
	{
		m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		m_colorBlendAttachment.blendEnable = VK_TRUE;
		m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	void PipelineBuilder::EnableBlendingAlphaBlend()
	{
		m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		m_colorBlendAttachment.blendEnable = VK_TRUE;
		m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	void PipelineBuilder::SetColorAttachmentFormat(VkFormat format)
	{
		m_colorAttachmentFormat = format;
		m_renderInfo.colorAttachmentCount = 1;
		m_renderInfo.pColorAttachmentFormats = &m_colorAttachmentFormat;
	}

	void PipelineBuilder::SetDepthFormat(VkFormat format)
	{
		m_renderInfo.depthAttachmentFormat = format;
	}

	void PipelineBuilder::DisableDepthTest()
	{
		m_depthStencil.depthTestEnable = VK_FALSE;
		m_depthStencil.depthWriteEnable = VK_FALSE;
		m_depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
		m_depthStencil.depthBoundsTestEnable = VK_FALSE;
		m_depthStencil.stencilTestEnable = VK_FALSE;
		m_depthStencil.front = {};
		m_depthStencil.back = {};
		m_depthStencil.minDepthBounds = 0.f;
		m_depthStencil.maxDepthBounds = 1.f;
	}

	void PipelineBuilder::EnableDepthTest(bool depthWriteEnable, VkCompareOp op)
	{
		m_depthStencil.depthTestEnable = VK_TRUE;
		m_depthStencil.depthWriteEnable = depthWriteEnable;
		m_depthStencil.depthCompareOp = op;
		m_depthStencil.depthBoundsTestEnable = VK_FALSE;
		m_depthStencil.stencilTestEnable = VK_FALSE;
		m_depthStencil.front = {};
		m_depthStencil.back = {};
		m_depthStencil.minDepthBounds = 0.f;
		m_depthStencil.maxDepthBounds = 1.f;
	}
}
