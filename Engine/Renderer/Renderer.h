#pragma once
#include "Shader.h"
#include "Vulkan/Device.h"
#include "RenderGraph.h"
#include "Resources/VulkanShader.h"
#include "../Core/Camera.h" 

namespace gns
{
	class RenderSystem;
	struct Mesh;
}
namespace gns::rendering {
	struct VulkanTexture;
	struct VulkanShader;

	class Renderer
	{
	private:
		Device m_device;
		RenderGraph m_renderGraph;
		std::vector<DrawData> m_drawData;
		CameraBackend m_cameraBackend;
		VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;
		VkDescriptorSetLayout _singleImageDescriptorLayout;
	public:

		Renderer() = default;
		~Renderer() = default;
		void CreateDevice(SDL_Window* sdl_window);
		void SetupRenderPasses();
		void DrawFrame(const std::vector<DrawData>& drawData, const GpuDataDescriptor* sceneDataDescriptor);
		
		GNS_API VkDevice GetDevice();
		GNS_API VkPhysicalDevice GetPhysicalDevice();
		GNS_API VkInstance GetInstance();
		GNS_API VkQueue GetGraphicsQueue();
		GNS_API VkFormat* GetSwapChainFormat();
		const VulkanDefaultTextureHandles& GetDefaultTextures() const;
		VulkanTexture* GetVulkanTexture(Handle textureHandle);
		VulkanShader* GetVulkanShader(Handle shaderHandle);
		VulkanMesh* GetVulkanMesh(Handle meshHandle);
		void WaitForIdle();
		
		Handle ApplyMesh(Mesh& mesh);
		Handle CreateVulkanShader(Shader& shader);
		friend class gns::RenderSystem;

	private:
		void AddBackgroundPass();
		void AddDrawImageToColorAttachmentPass();
		void AddGeometryPass();
		void AddCopyDrawImageToSwapchainPass();
		void AddImGuiPass();
	};
}

