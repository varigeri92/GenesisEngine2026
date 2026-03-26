#include "gnspch.h"
#include "Renderer.h"
#include "../Log/Logger.h"
#include "Vulkan/vkutils.h"

void gns::rendering::Renderer::CreateDevice(SDL_Window* sdl_window)
{
	m_device.Create(sdl_window);
	
	auto& backgroundPass = m_device.CreateRenderPass("background pass", 
		[&](VkCommandBuffer cmd, RenderPassData& rp_data)
	{
		//TRANSITION: RenderTarget F:X -> F:General (for draw)
		utils::TransitionImage(cmd, rp_data.renderTarget->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
		m_device.DrawTest(cmd);
		//TRANSITION: RenderTarget F:General -> F:ColorAttachment (for drawing geometry)
		utils::TransitionImage(cmd, rp_data.renderTarget->image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			
		return rp_data.randomBool;
	});
	backgroundPass.data.renderTarget = &m_device.m_drawImage;
	
	auto& geometryPass = m_device.CreateRenderPass("geometry pass", 
		[&](VkCommandBuffer cmd, RenderPassData& rp_data)
	{

		m_device.DrawGeometry(cmd);
		//TRANSITION: RenderTarget F:ColorAttachment -> F:TransferSRC (for copy to swapchain.)
		utils::TransitionImage(cmd, m_device.m_drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		//TRANSITION: Swapchain F:X -> F:Transfer_DST
		utils::TransitionImage(cmd, m_device.m_swapchain.GetImage(swapchainImageIndex), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		//COPY(Transfer): RenderTarget SRC:RenderTarget -> DST:Swapchain
		utils::CopyImageToImage(cmd, m_device.m_drawImage.image, m_device.m_swapchain.GetImage(swapchainImageIndex), 
	extent, m_device.m_swapchain.GetExtent());
		return rp_data.randomBool;
	});
	geometryPass.data.renderTarget = &m_device.m_drawImage;
	
	
	auto& imguiPass = m_device.CreateRenderPass("test pass", 
	[&](VkCommandBuffer cmd, RenderPassData& rp_data)
	{
		//TRANSITION: Swapchain F:Tranfer_DST -> F:ColorAttachment (Ready for imgui Draw)
	utils::TransitionImage(cmd, m_device.m_swapchain.GetImage(swapchainImageIndex), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	
	VkRenderingAttachmentInfo colorAttachment = 
		utils::AttachmentInfo( m_device.m_swapchain.GetImageView(swapchainImageIndex), nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	const VkRenderingInfo renderInfo = utils::RenderingInfo(m_device.m_swapchain.GetExtent(), &colorAttachment, nullptr);
	//draw imgui into the swapchain image
	gns::gui::GuiBackend::DrawImGui(cmd, renderInfo);

	//TRANSITION: Swapchain F:ColorAttachment -> F:Preset_optimal (presentation)
	utils::TransitionImage(cmd, m_device.m_swapchain.GetImage(swapchainImageIndex), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		return rp_data.randomBool;
	});
	imguiPass.data.renderTarget = &m_device.m_drawImage;
}

void gns::rendering::Renderer::DrawFrame()
{
    VkCommandBuffer cmd;
    m_device.BeginFrame(cmd, swapchainImageIndex, extent, data);
	m_device.ExecuteRenderPasses(cmd);
	m_device.DrawFrame(cmd, swapchainImageIndex, extent, data);
	m_device.EndFrame(cmd, swapchainImageIndex, extent, data);
	//CreateDrawData
	//BeginFrame
	
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
