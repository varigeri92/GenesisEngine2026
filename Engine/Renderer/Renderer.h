#pragma once
#include "Vulkan/Device.h"

namespace gns::rendering {
	class Renderer
	{

	public:
		Device m_device;

		Renderer() = default;
		~Renderer() = default;
		void CreateDevice(SDL_Window* sdl_window);
		void DrawFrame();
		
		GNS_API VkDevice GetDevice();
		GNS_API VkPhysicalDevice GetPhysicalDevice();
		GNS_API VkInstance GetInstance();
		GNS_API VkQueue GetGraphicsQueue();
		GNS_API VkFormat* GetSwapChainFormat();
		void WaitForIdle();
	};
}

