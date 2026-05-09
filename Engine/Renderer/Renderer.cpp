#include "gnspch.h"
#include "Renderer.h"
#include "../Log/Logger.h"
#include "../Object/Mesh.h"
#include "../Object/Texture.h"
#include "Vulkan/PipelineBuilder.h"
#include "Vulkan/ShaderUtils.h"
#include "Vulkan/vkutils.h"
#include "Vulkan/vulkan_log.h"
#include "Resources/VulkanShader.h"
#include "Resources/VulkanTexture.h"

#include <array>

void gns::rendering::Renderer::CreateDevice(SDL_Window* sdl_window)
{
	m_drawData = {};
	m_device.Create(sdl_window);
	const VkExtent2D renderExtent = m_device.GetRenderExtent();
	m_screen.SetSize(renderExtent.width, renderExtent.height);
	SetupRenderPasses();
}

void gns::rendering::Renderer::SetupRenderPasses()
{
	m_renderGraph.Clear();

	AddDrawImageToGeneralPass();
	AddBackgroundPass();
	AddDrawImageToColorAttachmentPass();
	AddGeometryPass();
	if (m_copySceneToSwapchain)
	{
		AddClearSwapchainPass(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		AddCopyDrawImageToSwapchainPass();
	}
	else
	{
		AddDrawImageToShaderReadPass();
		AddClearSwapchainPass(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}
	AddImGuiPass();
}

void gns::rendering::Renderer::AddDrawImageToGeneralPass()
{
	auto& transitionToGeneral = m_renderGraph.AddPass("DrawImageToGeneral",
	[&](VkCommandBuffer cmd, RenderStepData& rp_data, FrameData& frameData)
	{
		m_device.TransitionDrawImage(cmd, VK_IMAGE_LAYOUT_GENERAL);
		return true;
	});
	transitionToGeneral.data.renderTarget = m_device.GetRenderTarget();
}

void gns::rendering::Renderer::AddBackgroundPass()
{
	auto& backgroundPass = m_renderGraph.AddPass("Background",
		[&](VkCommandBuffer cmd, RenderStepData& rp_data,  FrameData& frameData)
	{
		m_device.DrawBackground(cmd);
		return true;
	});
	backgroundPass.data.renderTarget = m_device.GetRenderTarget();
}

void gns::rendering::Renderer::AddDrawImageToColorAttachmentPass()
{
	auto& transitionToColorAttachment = m_renderGraph.AddPass("DrawImageToColorAttachment",
	[&](VkCommandBuffer cmd, RenderStepData& rp_data,  FrameData& frameData)
	{
		m_device.TransitionDrawImage(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		m_device.TransitionDepthImage(cmd, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
		return true;
	});
	transitionToColorAttachment.data.srcImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	transitionToColorAttachment.data.dstImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	transitionToColorAttachment.data.renderTarget = m_device.GetRenderTarget();
	transitionToColorAttachment.data.depthTarget = m_device.GetDepthTarget();
}

void gns::rendering::Renderer::AddGeometryPass()
{
	auto& geometryPass = m_renderGraph.AddPass("Geometry",
		[&](VkCommandBuffer cmd, RenderStepData& rp_data, FrameData& frameData)
	{
		m_device.DrawGeometry(cmd);
		for (auto& drawData : m_drawData)
		{
			m_device.DrawMesh(cmd, drawData);
		}
		m_device.EndRendering(cmd);
		return true;
	});
	geometryPass.data.renderTarget = m_device.GetRenderTarget();
}

void gns::rendering::Renderer::AddCopyDrawImageToSwapchainPass()
{
	auto& copyToSwapchain = m_renderGraph.AddPass("CopyDrawImageToSwapchain",
	[&](VkCommandBuffer cmd, RenderStepData& rp_data,  FrameData& frameData)
	{
		m_device.TransitionDrawImage(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		utils::CopyImageToImage(
			cmd, rp_data.renderTarget->image, frameData._swapchain->GetImage(frameData._swapchainImageIndex), 
	{rp_data.renderTarget->imageExtent.width, rp_data.renderTarget->imageExtent.height}, 
	frameData._swapchain->GetExtent());
		m_device.TransitionDrawImage(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		return true;
	});
	copyToSwapchain.data.srcImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	copyToSwapchain.data.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	copyToSwapchain.data.renderTarget = m_device.GetRenderTarget();
}

void gns::rendering::Renderer::AddClearSwapchainPass(VkImageLayout finalLayout)
{
	auto& clearSwapchain = m_renderGraph.AddPass("ClearSwapchain",
	[finalLayout](VkCommandBuffer cmd, RenderStepData& rp_data, FrameData& frameData)
	{
		utils::TransitionImage(
			cmd,
			frameData._swapchain->GetImage(frameData._swapchainImageIndex),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkClearValue clearColor = {};
		clearColor.color = { { 0.f, 0.f, 0.f, 1.f } };

		VkRenderingAttachmentInfo colorAttachment =
			utils::AttachmentInfo(
				frameData._swapchain->GetImageView(frameData._swapchainImageIndex),
				&clearColor,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		const VkRenderingInfo renderInfo =
			utils::RenderingInfo(frameData._swapchain->GetExtent(), &colorAttachment, nullptr);

		vkCmdBeginRendering(cmd, &renderInfo);
		vkCmdEndRendering(cmd);

		if (finalLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			utils::TransitionImage(
				cmd,
				frameData._swapchain->GetImage(frameData._swapchainImageIndex),
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				finalLayout);
		}

		return true;
	});
	clearSwapchain.data.dstImageLayout = finalLayout;
}

void gns::rendering::Renderer::AddDrawImageToShaderReadPass()
{
	auto& shaderReadPass = m_renderGraph.AddPass("DrawImageToShaderRead",
	[&](VkCommandBuffer cmd, RenderStepData& rp_data, FrameData& frameData)
	{
		m_device.TransitionDrawImage(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		return true;
	});
	shaderReadPass.data.renderTarget = m_device.GetRenderTarget();
}

void gns::rendering::Renderer::AddImGuiPass()
{
	auto& imguiPass = m_renderGraph.AddPass("ImGui",
	[&](VkCommandBuffer cmd, RenderStepData& rp_data, FrameData& frameData)
	{
		if (rp_data.srcImageLayout != rp_data.dstImageLayout)
		{
			utils::TransitionImage(
				cmd,
				frameData._swapchain->GetImage(frameData._swapchainImageIndex),
				rp_data.srcImageLayout,
				rp_data.dstImageLayout);
		}
		VkRenderingAttachmentInfo colorAttachment = 
			utils::AttachmentInfo( frameData._swapchain->GetImageView(frameData._swapchainImageIndex), nullptr, rp_data.dstImageLayout);
		const VkRenderingInfo renderInfo = utils::RenderingInfo(frameData._swapchain->GetExtent(), &colorAttachment, nullptr);
		gns::gui::GuiBackend::DrawImGui(cmd, renderInfo);
		utils::TransitionImage(cmd, frameData._swapchain->GetImage(frameData._swapchainImageIndex), rp_data.dstImageLayout, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		return true;
	});
	imguiPass.data.renderTarget = m_device.GetRenderTarget();
	imguiPass.data.srcImageLayout = m_copySceneToSwapchain
		? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		: VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imguiPass.data.dstImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

void gns::rendering::Renderer::DrawFrame(
	const std::vector<DrawData>& drawData,
	const GpuDataDescriptor* sceneDataDescriptor)
{
	if (m_device.m_resizeRequest)
		m_device.ResizeSwapchain();
	m_device.ApplyRenderTargetResize();
	
	VkCommandBuffer cmd;
	uint32_t swapchainImageIndex;
	VkExtent2D extent;
	FrameData frameData;
    if (m_device.BeginFrame(cmd, swapchainImageIndex, extent, frameData))
	{
		m_drawData = drawData;
		if (sceneDataDescriptor != nullptr && !m_drawData.empty())
		{
			if (_gpuSceneDataDescriptorLayout == VK_NULL_HANDLE)
			{
				LOG_WARNING("[Renderer]: Cannot update scene data descriptor because layout is null.");
				m_drawData.clear();
			}
			else
			{
				m_device.UpdateDescriptorSet(*sceneDataDescriptor, _gpuSceneDataDescriptorLayout);
			}
		}
		m_renderGraph.Execute(cmd, frameData);
		//depth prepass
		//Shadows prepass
		//culling pass
		//geometry pass
			//Opaque pass
			//transparent pass
		//PostProcessing Pass
		//GuiPass <- gameGUI
		//imgui pass
		//present
		m_device.EndFrame(cmd, swapchainImageIndex, extent, frameData);
    }
}

VkDevice gns::rendering::Renderer::GetDevice()
{
	return m_device.GetDevice();
}
VkPhysicalDevice gns::rendering::Renderer::GetPhysicalDevice()
{
	return m_device.GetGPU();
}

VkInstance gns::rendering::Renderer::GetInstance()
{
	return m_device.GetInstance();
}

VkQueue gns::rendering::Renderer::GetGraphicsQueue()
{
	return m_device.GetGraphicsQueue();
}

VkFormat* gns::rendering::Renderer::GetSwapChainFormat()
{
	return m_device.GetSwapchain().GetFormat_ptr();
}

uint64_t gns::rendering::Renderer::GetSceneTextureDescriptor()
{
	return m_device.GetRenderTargetDescriptor();
}

void gns::rendering::Renderer::SetScreen(const Screen& screen)
{
	if (!screen.IsValid())
	{
		return;
	}

	m_screen = screen;
	m_device.SetRenderExtent({ screen.GetWidth(), screen.GetHeight() });
}

const gns::rendering::VulkanDefaultTextureHandles& gns::rendering::Renderer::GetDefaultTextures() const
{
	return m_device.GetDefaultTextures();
}

gns::rendering::VulkanTexture* gns::rendering::Renderer::GetVulkanTexture(Handle textureHandle)
{
	return m_device.GetResource<VulkanTexture>(textureHandle);
}

gns::rendering::VulkanShader* gns::rendering::Renderer::GetVulkanShader(Handle shaderHandle)
{
	return m_device.GetResource<VulkanShader>(shaderHandle);
}

VulkanMesh* gns::rendering::Renderer::GetVulkanMesh(Handle meshHandle)
{
	return m_device.GetResource<VulkanMesh>(meshHandle);
}

void gns::rendering::Renderer::WaitForIdle()
{
	m_device.WaitForIdle();
}

gns::Handle gns::rendering::Renderer::ApplyMesh(Mesh& mesh)
{
	std::vector<Vertex> vertices = {};
	vertices.reserve(mesh.positions.size());
	for (size_t i = 0; i < mesh.positions.size(); ++i)
	{
		vertices.emplace_back(mesh.positions[i], mesh.uvs[i].x, mesh.normals[i],mesh.uvs[i].y, mesh.colors[i]);
	}
	VulkanMesh& vulkan_mesh = VulkanMesh::UploadMesh(m_device, m_device.GetAlocator(), mesh.indices, vertices);
	vulkan_mesh.startIndex = mesh.bufferRange.startIndex;
	vulkan_mesh.count = mesh.bufferRange.count;
	return vulkan_mesh.GetHandle();
}

gns::Handle gns::rendering::Renderer::ApplyTexture(Texture& texture)
{
	if (!texture.HasPixels() || texture.width == 0 || texture.height == 0)
	{
		LOG_ERROR("[Renderer]: Cannot apply texture without pixel data.");
		LOG_ERROR(texture.GetName());
		return {};
	}

	if (texture.format != TextureFormat::R8G8B8A8_UNorm)
	{
		LOG_ERROR("[Renderer]: Unsupported texture format.");
		LOG_ERROR(texture.GetName());
		return {};
	}

	VulkanTexture* vulkanTexture = m_device.CreateResource<VulkanTexture>();
	if (vulkanTexture == nullptr)
	{
		LOG_ERROR("[Renderer]: Failed to create Vulkan texture resource.");
		LOG_ERROR(texture.GetName());
		return {};
	}

	const VkSamplerCreateInfo samplerInfo = utils::SamplerCreateInfo(VK_FILTER_LINEAR);
	VK_CHECK(vkCreateSampler(m_device.GetDevice(), &samplerInfo, nullptr, &vulkanTexture->sampler));

	vulkanTexture->CreateTexture(
		texture.pixels.data(),
		VkExtent3D{ texture.width, texture.height, 1 },
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		false);
	m_device.CreateTextureDescriptor(*vulkanTexture);
	if (vulkanTexture->descriptorSet == VK_NULL_HANDLE)
	{
		LOG_ERROR("[Renderer]: Failed to create texture descriptor.");
		LOG_ERROR(texture.GetName());
		return {};
	}

	return vulkanTexture->GetHandle();
}

gns::Handle gns::rendering::Renderer::CreateVulkanShader(Shader& shader)
{
	VulkanShader& vkShader = *m_device.CreateResource<VulkanShader>();
	std::vector<ShaderReflectionData> shaderReflections;

	ShaderReflectionData vertexReflection;
	if (ShaderUtils::ReflectShaderFile(
		ShaderUtils::ResolveCompiledShaderPath(shader.GetVertexShaderPath()),
		vertexReflection))
	{
		ShaderUtils::PrintReflection(vertexReflection);
		shaderReflections.emplace_back(vertexReflection);
	}

	ShaderReflectionData fragmentReflection;
	if (ShaderUtils::ReflectShaderFile(
		ShaderUtils::ResolveCompiledShaderPath(shader.GetFragmentShaderPath()),
		fragmentReflection))
	{
		ShaderUtils::PrintReflection(fragmentReflection);
		shaderReflections.emplace_back(fragmentReflection);
	}

	if (shaderReflections.empty())
	{
		LOG_ERROR("[Renderer]: Cannot create shader pipeline without SPIR-V reflection data.");
		LOG_ERROR(shader.GetName());
		return {};
	}
	
	std::vector<VkPushConstantRange> pushConstantRanges =
		ShaderUtils::BuildPushConstantRanges(shaderReflections);
	if (pushConstantRanges.empty())
	{
		VkPushConstantRange defaultPushConstant{};
		defaultPushConstant.offset = 0;
		defaultPushConstant.size = sizeof(GPUDrawPushConstants);
		defaultPushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRanges.emplace_back(defaultPushConstant);
	}

	if (!ShaderUtils::CreateDescriptorSetLayouts(
		m_device.GetDevice(),
		shaderReflections,
		vkShader.m_descriptorSetLayouts))
	{
		LOG_ERROR("[Renderer]: Failed to build descriptor set layouts from shader reflection.");
		return {};
	}

	if (!vkShader.m_descriptorSetLayouts.empty())
	{
		vkShader.m_descriptorSetLayout = vkShader.m_descriptorSetLayouts[0];
		_gpuSceneDataDescriptorLayout = vkShader.m_descriptorSetLayout;
	}

	VkPipelineLayoutCreateInfo pipeline_layout_info = utils::PipelineLayoutCreateInfo();
	pipeline_layout_info.pPushConstantRanges = pushConstantRanges.empty() ? nullptr : pushConstantRanges.data();
	pipeline_layout_info.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
	pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(vkShader.m_descriptorSetLayouts.size());
	pipeline_layout_info.pSetLayouts = vkShader.m_descriptorSetLayouts.empty()
		? nullptr
		: vkShader.m_descriptorSetLayouts.data();
	VK_CHECK(vkCreatePipelineLayout(m_device.GetDevice(), &pipeline_layout_info, nullptr, &vkShader.m_pipelineLayout));
	
	
	PipelineBuilder builder{&m_device};
	builder.m_pipelineLayout = vkShader.m_pipelineLayout;
	builder.SetShaders(shader);
	builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	//filled triangles
	builder.SetPolygonMode(VK_POLYGON_MODE_FILL);
	//no backface culling
	builder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	//no multisampling
	builder.SetMultisampling();
	//no blending
	builder.DisableBlending();
	builder.EnableDepthTest(true,VK_COMPARE_OP_GREATER_OR_EQUAL);
	
	builder.SetColorAttachmentFormat(m_device.GetRenderTarget()->imageFormat);
	builder.SetDepthFormat(m_device.GetDepthTarget()->imageFormat);
	
	vkShader.m_pipeline = builder.BuildPipeline(m_device.GetDevice());
	return vkShader.GetHandle();
}
