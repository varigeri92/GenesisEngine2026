#pragma once
#include <functional>
#include <queue>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <vector>
#include "Swapchain.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "VulkanImage.h"

namespace gns::rendering 
{
	class Device
	{
		
		struct CleanupQueue
		{
			CleanupQueue() = default;
			~CleanupQueue() = default;
		private:
			std::deque<std::function<void()>> m_queue;
		public:
			void Push(std::function<void()>&& func);
			void Flush();	
		};

		struct FrameData {
			VkCommandPool _commandPool;
			VkCommandBuffer _mainCommandBuffer;
			
			VkSemaphore _swapchainSemaphore; 
			VkSemaphore _renderSemaphore;
			VkFence _renderFence;
			
			CleanupQueue _cleanupQueue;
		};
		
	public:
		Device();
		~Device();

		void Create(SDL_Window* sdl_window);
		VkPhysicalDevice GetGPU() { return m_physDevice; };
		VkDevice GetDevice() { return m_device; };
		VkSurfaceKHR GetSurface() { return m_surface; };
		
		
		void DrawFrame();
	private:
		CleanupQueue m_cleanupQueue;
		VkInstance m_instance;
		VkDebugUtilsMessengerEXT m_debugMessenger;
		VkPhysicalDevice m_physDevice;
		VkDevice m_device;
		VkSurfaceKHR m_surface;
		Swapchain m_swapchain;
		VmaAllocator m_allocator;
		SDL_Window* m_sdl_window;

		void InitVulkan(SDL_Window* sdl_window);
		void InitSwapchain();
		void InitCommands();
		void InitSyncStructs();

		FrameData& GetCurrentFrame();
		FrameData& GetFrameByIndex(size_t index);
		void Cleanup();

		void DrawTest(VkCommandBuffer cmd);
		
		std::vector<FrameData> m_frames;
		VulkanImage m_drawImage;
		
		VkQueue m_graphicsQueue;
		uint32_t m_graphicsQueueFamily;

		VkQueue m_computeQueue;
		uint32_t m_computeQueueFamily;

		VkQueue m_transferQueue;
		uint32_t m_transferQueueFamily;

		VkQueue m_presentQueue;
		uint32_t m_presentQueueFamily;

		size_t m_currentFrame;
	};
}

