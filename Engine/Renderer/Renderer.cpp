#include "gnspch.h"
#include "Renderer.h"
#include "../Log/Logger.h"
#include "Vulkan/vkutils.h"

void gns::rendering::Renderer::CreateDevice(SDL_Window* sdl_window)
{
	m_device.Create(sdl_window);
	SetupRenderPasses();
}

void gns::rendering::Renderer::SetupRenderPasses()
{
	
	auto& transitionStep = m_device.CreateRenderPass("ImageTransition step",
		[&](VkCommandBuffer cmd, RenderStepData& rp_data,  FrameData& frameData)
	{
		utils::TransitionImage(
			cmd, rp_data.renderTarget->image, rp_data.srcImageLayout, rp_data.dstImageLayout);
		return true;
	});
	transitionStep.data.srcImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	transitionStep.data.dstImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	transitionStep.data.renderTarget = m_device.GetRenderTarget();
	
	auto& backgroundPass = m_device.CreateRenderPass("background pass", 
		[&](VkCommandBuffer cmd, RenderStepData& rp_data,  FrameData& frameData)
	{
		m_device.DrawTest(cmd);
		return rp_data.randomBool;
	});
	backgroundPass.data.renderTarget = m_device.GetRenderTarget();
	
	auto& transitionToColorAttachment = m_device.CreateRenderPass("ImageTransition step",
	[&](VkCommandBuffer cmd, RenderStepData& rp_data,  FrameData& frameData)
	{
		utils::TransitionImage(
			cmd, rp_data.renderTarget->image, rp_data.srcImageLayout, rp_data.dstImageLayout);
		return true;
	});
	transitionToColorAttachment.data.srcImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	transitionToColorAttachment.data.dstImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	transitionToColorAttachment.data.renderTarget = m_device.GetRenderTarget();
	
	auto& geometryPass = m_device.CreateRenderPass("geometry pass", 
		[&](VkCommandBuffer cmd, RenderStepData& rp_data, FrameData& frameData)
	{
		m_device.DrawGeometry(cmd);
		
		return rp_data.randomBool;
	});
	geometryPass.data.renderTarget = m_device.GetRenderTarget();
	
	auto& copyToSwapchain = m_device.CreateRenderPass("ImageTransition step",
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
	
	auto& imguiPass = m_device.CreateRenderPass("test pass", 
	[&](VkCommandBuffer cmd, RenderStepData& rp_data, FrameData& frameData)
	{
		utils::TransitionImage(cmd, frameData._swapchain->GetImage(frameData._swapchainImageIndex), rp_data.srcImageLayout, rp_data.dstImageLayout);
		VkRenderingAttachmentInfo colorAttachment = 
			utils::AttachmentInfo( frameData._swapchain->GetImageView(frameData._swapchainImageIndex), nullptr, rp_data.dstImageLayout);
		const VkRenderingInfo renderInfo = utils::RenderingInfo(frameData._swapchain->GetExtent(), &colorAttachment, nullptr);
		gns::gui::GuiBackend::DrawImGui(cmd, renderInfo);
		utils::TransitionImage(cmd, frameData._swapchain->GetImage(frameData._swapchainImageIndex), rp_data.dstImageLayout, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		return rp_data.randomBool;
	});
	imguiPass.data.renderTarget = m_device.GetRenderTarget();
	imguiPass.data.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imguiPass.data.dstImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

void gns::rendering::Renderer::BuildDrawData()
{
	
}

void gns::rendering::Renderer::DrawFrame()
{
	//CreateDrawData
	void BuildDrawData();
    VkCommandBuffer cmd;
	uint32_t swapchainImageIndex;
	VkExtent2D extent;
	FrameData frameData;
	//BeginFrame
    m_device.BeginFrame(cmd, swapchainImageIndex, extent, frameData);
	m_device.ExecuteRenderPasses(cmd, frameData);
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

void gns::rendering::Renderer::WaitForIdle()
{
	m_device.WaitForIdle();
}
