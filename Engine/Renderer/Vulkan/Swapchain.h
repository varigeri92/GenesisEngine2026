#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace gns::rendering
{
	class Device;

	class Swapchain
	{
	public:
		Swapchain();
		~Swapchain() = default;
		void CreateSwapchain(Device* m_device, VkExtent2D extent);
		void DestroySwapchain();
		void ResizeSwapchain();
		
		VkSwapchainKHR GetSwapchain() { return m_swapchain; };
		VkFormat GetFormat() { return m_swapchainImageFormat; };
		VkExtent2D GetExtent() { return m_swapchainExtent; };

	private:
		Device* m_device;
		VkSwapchainKHR m_swapchain;
		VkFormat m_swapchainImageFormat;
		VkExtent2D m_swapchainExtent;
		std::vector<VkImage> m_swapchainImages;
		std::vector<VkImageView> m_swapchainImageViews;
	};
}