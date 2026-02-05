#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "Swapchain.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

namespace gns::rendering 
{	
	constexpr unsigned int FRAME_OVERLAP = 2;
	class Device
	{
		struct FrameData {
			VkCommandPool _commandPool;
			VkCommandBuffer _mainCommandBuffer;
		};
	public:
		Device();
		~Device();

		void Create(SDL_Window* sdl_window);
		VkPhysicalDevice GetGPU() { return m_physDevice; };
		VkDevice GetDevice() { return m_device; };
		VkSurfaceKHR GetSurface() { return m_surface; };

	private:
		VkInstance m_instance;
		VkDebugUtilsMessengerEXT m_debugMessenger;
		VkPhysicalDevice m_physDevice;
		VkDevice m_device;
		VkSurfaceKHR m_surface;
		Swapchain m_swapchain;
	
		SDL_Window* m_sdl_window;

		void InitVulkan(SDL_Window* sdl_window);
		void InitSwapchain();
		void InitCommands();
		void InitSyncStructs();

		FrameData& GetCurrentFrame();
		
		void Cleanup();

		std::vector<FrameData> m_frames;
		
		VkQueue m_graphicsQueue;
		uint32_t m_graphicsQueueFamily;

		VkQueue m_computeQueue;
		uint32_t m_computeQueueFamily;

		VkQueue m_transferQueue;
		uint32_t m_transferQueueFamily;

		VkQueue m_presentQueue;
		uint32_t m_presentQueueFamily;

	};
}

