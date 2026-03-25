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
		
		VkSwapchainKHR GetSwapchain() const { return m_swapchain; }
		VkSwapchainKHR* GetSwapchain_ptr() { return &m_swapchain; }
		VkFormat GetFormat() const { return m_swapchainImageFormat; }
		VkFormat* GetFormat_ptr() { return &m_swapchainImageFormat; }
		VkExtent2D GetExtent() const { return m_swapchainExtent; }
		VkImage GetImage(uint32_t index) const { return m_swapchainImages[index]; }
		VkImageView GetImageView(uint32_t index) const { return m_swapchainImageViews[index]; }

	private:
		Device* m_device;
		VkSwapchainKHR m_swapchain;
		VkFormat m_swapchainImageFormat;
		VkExtent2D m_swapchainExtent;
		std::vector<VkImage> m_swapchainImages;
		std::vector<VkImageView> m_swapchainImageViews;
		std::vector<VkSemaphore> m_swapchainRenderSemaphores; 
		std::vector<VkFence>     m_SwapchainImageFences;         

	};
}