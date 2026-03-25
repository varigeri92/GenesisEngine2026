#include "gnspch.h"
#include "Renderer.h"
#include "../Log/Logger.h"

void gns::rendering::Renderer::CreateDevice(SDL_Window* sdl_window)
{
	m_device.Create(sdl_window);
}

void gns::rendering::Renderer::DrawFrame()
{
	m_device.DrawFrame();
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
