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
		
		VkDevice GetDevice();
		VkPhysicalDevice GetPhysicalDevice();
		VkInstance GetInstance();
		VkQueue GetGraphicsQueue();
		VkFormat GetSwapChainFormat();
	};
}

