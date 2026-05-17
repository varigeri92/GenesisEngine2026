#pragma once
#include "Shader.h"
#include "Vulkan/Device.h"
#include "RenderGraph.h"
#include "Resources/VulkanShader.h"
#include "../Core/Camera.h" 
#include "../Core/Screen.h"

namespace gns
{
	class RenderSystem;
	struct Mesh;
	struct Texture;
}
namespace gns::rendering {
	struct VulkanTexture;
	struct VulkanShader;
	struct VulkanMaterial;

	class Renderer
	{
	private:
		Device m_device;
		RenderGraph m_renderGraph;
		RenderFramePacket m_framePacket;
		CameraBackend m_cameraBackend;
		Screen m_screen;
		bool m_copySceneToSwapchain = false;
		VkDescriptorSetLayout _singleImageDescriptorLayout;
	public:

		Renderer() = default;
		~Renderer() = default;
		void CreateDevice(SDL_Window* sdl_window);
		void SetupRenderPasses();
		void DrawFrame(const RenderFramePacket& packet);
		
		GNS_API VkDevice GetDevice();
		GNS_API VkPhysicalDevice GetPhysicalDevice();
		GNS_API VkInstance GetInstance();
		GNS_API VkQueue GetGraphicsQueue();
		GNS_API VkFormat* GetSwapChainFormat();
		GNS_API uint64_t GetSceneTextureDescriptor();
		GNS_API void SetScreen(const Screen& screen);
		const VulkanDefaultTextureHandles& GetDefaultTextures() const;
		VulkanTexture* GetVulkanTexture(Handle textureHandle);
		VulkanShader* GetVulkanShader(Handle shaderHandle);
		VulkanMaterial* GetVulkanMaterial(Handle materialHandle);
		VulkanMesh* GetVulkanMesh(Handle meshHandle);
		void WaitForIdle();
		
		Handle ApplyMesh(Mesh& mesh);
		Handle ApplyTexture(Texture& texture);
		Handle CreateVulkanShader(Shader& shader);
		friend class gns::RenderSystem;

	private:
		void AddBackgroundPass();
		void AddDrawImageToGeneralPass();
		void AddDrawImageToColorAttachmentPass();
		void AddGeometryPass();
		void AddClearSwapchainPass(VkImageLayout finalLayout);
		void AddCopyDrawImageToSwapchainPass();
		void AddDrawImageToShaderReadPass();
		void AddImGuiPass();
	};
}

