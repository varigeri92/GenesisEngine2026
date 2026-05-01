#include "gnspch.h"
#include "Renderer.h"
#include "../Log/Logger.h"
#include "../Object/Mesh.h"
#include "Vulkan/PipelineBuilder.h"
#include "Vulkan/vkutils.h"
#include "Vulkan/vulkan_log.h"
#include "Resources/VulkanShader.h"
#include "Resources/VulkanTexture.h"

#include <array>

void gns::rendering::Renderer::CreateDevice(SDL_Window* sdl_window)
{
	m_drawData = {};
	m_device.Create(sdl_window);
	SetupRenderPasses();
}

void gns::rendering::Renderer::SetupRenderPasses()
{
	m_renderGraph.Clear();

	m_renderGraph.AddImageTransitionPass(
		"DrawImageToGeneral",
		m_device.GetRenderTarget(),
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL);
	AddBackgroundPass();
	AddDrawImageToColorAttachmentPass();
	AddGeometryPass();
	AddCopyDrawImageToSwapchainPass();
	AddImGuiPass();
}

void gns::rendering::Renderer::AddBackgroundPass()
{
	auto& backgroundPass = m_renderGraph.AddPass("Background",
		[&](VkCommandBuffer cmd, RenderStepData& rp_data,  FrameData& frameData)
	{
		m_device.DrawTest(cmd);
		return true;
	});
	backgroundPass.data.renderTarget = m_device.GetRenderTarget();
}

void gns::rendering::Renderer::AddDrawImageToColorAttachmentPass()
{
	auto& transitionToColorAttachment = m_renderGraph.AddPass("DrawImageToColorAttachment",
	[&](VkCommandBuffer cmd, RenderStepData& rp_data,  FrameData& frameData)
	{
		utils::TransitionImage(
			cmd, rp_data.renderTarget->image, rp_data.srcImageLayout, rp_data.dstImageLayout);
		utils::TransitionImage(cmd,  rp_data.depthTarget->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
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
		utils::TransitionImage(
			cmd, rp_data.renderTarget->image, rp_data.srcImageLayout, rp_data.dstImageLayout);
		utils::TransitionImage(
			cmd, frameData._swapchain->GetImage(frameData._swapchainImageIndex), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		utils::CopyImageToImage(
			cmd, rp_data.renderTarget->image, frameData._swapchain->GetImage(frameData._swapchainImageIndex), 
	{rp_data.renderTarget->imageExtent.width, rp_data.renderTarget->imageExtent.height}, 
	frameData._swapchain->GetExtent());
		return true;
	});
	copyToSwapchain.data.srcImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	copyToSwapchain.data.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	copyToSwapchain.data.renderTarget = m_device.GetRenderTarget();
}

void gns::rendering::Renderer::AddImGuiPass()
{
	auto& imguiPass = m_renderGraph.AddPass("ImGui",
	[&](VkCommandBuffer cmd, RenderStepData& rp_data, FrameData& frameData)
	{
		utils::TransitionImage(cmd, frameData._swapchain->GetImage(frameData._swapchainImageIndex), rp_data.srcImageLayout, rp_data.dstImageLayout);
		VkRenderingAttachmentInfo colorAttachment = 
			utils::AttachmentInfo( frameData._swapchain->GetImageView(frameData._swapchainImageIndex), nullptr, rp_data.dstImageLayout);
		const VkRenderingInfo renderInfo = utils::RenderingInfo(frameData._swapchain->GetExtent(), &colorAttachment, nullptr);
		gns::gui::GuiBackend::DrawImGui(cmd, renderInfo);
		utils::TransitionImage(cmd, frameData._swapchain->GetImage(frameData._swapchainImageIndex), rp_data.dstImageLayout, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		return true;
	});
	imguiPass.data.renderTarget = m_device.GetRenderTarget();
	imguiPass.data.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imguiPass.data.dstImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

void gns::rendering::Renderer::DrawFrame(
	const std::vector<DrawData>& drawData,
	const GpuDataDescriptor* sceneDataDescriptor)
{
	if (m_device.m_resizeRequest)
		m_device.ResizeSwapchain();
	
	VkCommandBuffer cmd;
	uint32_t swapchainImageIndex;
	VkExtent2D extent;
	FrameData frameData;
    if (m_device.BeginFrame(cmd, swapchainImageIndex, extent, frameData))
    {
		m_drawData = drawData;
		if (sceneDataDescriptor != nullptr)
		{
			m_device.UpdateDescriptorSet(*sceneDataDescriptor, _gpuSceneDataDescriptorLayout);
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

gns::Handle gns::rendering::Renderer::CreateVulkanShader(Shader& shader)
{
	VulkanShader& vkShader = *m_device.CreateResource<VulkanShader>();
	
	VkPushConstantRange bufferRange{};
	bufferRange.offset = 0;
	bufferRange.size = sizeof(GPUDrawPushConstants);
	bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		vkShader.m_descriptorSetLayout =
			builder.Build(m_device.GetDevice(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		_gpuSceneDataDescriptorLayout = vkShader.m_descriptorSetLayout;
	}
	
	VkPipelineLayoutCreateInfo pipeline_layout_info = utils::PipelineLayoutCreateInfo();
	std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts =
	{
		vkShader.m_descriptorSetLayout,
		m_device.GetTextureDescriptorLayout()
	};
	pipeline_layout_info.pPushConstantRanges = &bufferRange;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipeline_layout_info.pSetLayouts = descriptorSetLayouts.data();
	VK_CHECK(vkCreatePipelineLayout(m_device.GetDevice(), &pipeline_layout_info, nullptr, &vkShader.m_pipelineLayout));
	
	
	PipelineBuilder builder{&m_device};
	builder.m_pipelineLayout = vkShader.m_pipelineLayout;
	builder.SetShaders(shader);
	builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	//filled triangles
	builder.SetPolygonMode(VK_POLYGON_MODE_FILL);
	//no backface culling
	builder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
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
