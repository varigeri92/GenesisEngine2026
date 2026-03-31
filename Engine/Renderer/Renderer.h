#pragma once
#include "Shader.h"
#include "Vulkan/Device.h"
#include "Resources/VulkanShader.h"

namespace gns
{
	struct Mesh;
}
namespace gns::rendering {
	class Renderer
	{
	private:
		Device m_device;
		std::vector<DrawData> m_drawData;
	public:

		Renderer() = default;
		~Renderer() = default;
		void CreateDevice(SDL_Window* sdl_window);
		void SetupRenderPasses();
		void BuildDrawData();
		void DrawFrame();
		
		GNS_API VkDevice GetDevice();
		GNS_API VkPhysicalDevice GetPhysicalDevice();
		GNS_API VkInstance GetInstance();
		GNS_API VkQueue GetGraphicsQueue();
		GNS_API VkFormat* GetSwapChainFormat();
		void WaitForIdle();
		
		void ApplyMesh(Mesh& mesh);
		void CreateVulkanShader(Shader& shader);
	};
}

