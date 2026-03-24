#include "gnspch.h"
#include "Swapchain.h"
#include <VkBootstrap.h>
#include "Device.h"
#include "vkutils.h"
#include "vulkan_log.h"

constexpr uint32_t _w = 1920;
constexpr uint32_t _h = 1080;

gns::rendering::Swapchain::Swapchain() :
	m_device(nullptr),
	m_swapchain(VK_NULL_HANDLE),
	m_swapchainImageFormat(VK_FORMAT_B8G8R8A8_UNORM),
	m_swapchainExtent{1, 1},
	m_swapchainImages(), 
	m_swapchainImageViews(),
	m_swapchainRenderSemaphores(),
	m_SwapchainImageFences()
{}

void gns::rendering::Swapchain::CreateSwapchain(Device* device, VkExtent2D extent)
{
	m_device = device;
	vkb::SwapchainBuilder swapchainBuilder{ 
		m_device->GetGPU(),
		m_device->GetDevice(),
		m_device->GetSurface(), 
	};

	m_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		//.use_default_format_selection()
		.set_desired_format(VkSurfaceFormatKHR{ .format = m_swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
		.set_desired_extent(extent.width, extent.height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	m_swapchainExtent = vkbSwapchain.extent;
	//store swapchain and its related images
	m_swapchain = vkbSwapchain.swapchain;
	m_swapchainImages = vkbSwapchain.get_images().value();
	m_swapchainImageViews = vkbSwapchain.get_image_views().value();

	LOG_INFO("Swapchain Created!");
}



void gns::rendering::Swapchain::DestroySwapchain()
{
	vkDestroySwapchainKHR(m_device->GetDevice(), m_swapchain, nullptr);
	for (int i = 0; i < m_swapchainImageViews.size(); i++) {

		vkDestroyImageView(m_device->GetDevice(), m_swapchainImageViews[i], nullptr);
	}
}
