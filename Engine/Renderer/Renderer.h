#pragma once
#include "Shader.h"
#include "Vulkan/Device.h"
#include "Resources/VulkanShader.h"
#include "../Core/Camera.h" 

namespace gns
{
	class RenderSystem;
	struct Mesh;
}
namespace gns::rendering {
	class Renderer
	{
	private:
		Device m_device;
		std::vector<DrawData> m_drawData;
		CameraBackend m_cameraBackend;
		VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;
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
		friend class gns::RenderSystem;
	};
}

