#pragma once
#include <functional>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <vector>
#include "Swapchain.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include "DescriptorLayoutBuilder.h"
#include "VulkanImage.h"
#include "../Gui/GuiBackend.h"
#include "VulkanMesh.h"
#include <glm/glm.hpp>
#include "DrawData.h"
namespace gns::rendering 
{
	struct RenderStepData;
	struct RenderStep;
	struct VulkanShader;
	
	struct CleanupQueue
	{
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
		
		Swapchain* _swapchain;
		uint32_t _swapchainImageIndex;
		DescriptorAllocatorGrowable _frameDescriptors;
	};
	class Device
	{
		struct ComputePushConstants {
			glm::vec4 data1;
			glm::vec4 data2;
			glm::vec4 data3;
			glm::vec4 data4;
		};
		
		std::vector<RenderStep> renderPasses;
	public:
		
		Device();
		~Device();

		void Create(SDL_Window* sdl_window);
		VkPhysicalDevice GetGPU() { return m_physDevice; }
		void WaitForIdle();
		VkDevice GetDevice() { return m_device; }
		VkSurfaceKHR GetSurface() { return m_surface; }
		VkInstance GetInstance() { return m_instance; }
		VkQueue GetGraphicsQueue() { return m_graphicsQueue; }
		Swapchain GetSwapchain() { return m_swapchain; }
		VmaAllocator GetAlocator(){return m_allocator;}

		VkFence m_immediateFence = VK_NULL_HANDLE;
		VkCommandBuffer m_immediateCommandBuffer = VK_NULL_HANDLE;
		VkCommandPool m_immediateCommandPool = VK_NULL_HANDLE;
		
		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
		bool BeginFrame(
			VkCommandBuffer& cmd, uint32_t& swapchainImageIndex, VkExtent2D& extent, FrameData& data);
		void DrawFrame(VkCommandBuffer& cmd, uint32_t& swapchainImageIndex, VkExtent2D& extent , FrameData& data);
		void ExecuteRenderPasses(VkCommandBuffer& cmd, FrameData& frameData);
		void EndFrame(VkCommandBuffer& cmd, uint32_t& swapchainImageIndex, VkExtent2D& extent , FrameData& data);
		RenderStep& CreateRenderPass(std::string name, 
			std::function<bool(VkCommandBuffer, RenderStepData&,  FrameData&)> renderPassFunction);
		VulkanImage* GetRenderTarget() { return &m_drawImage; }
		VulkanImage* GetDepthTarget() { return &m_depthImage; }

		void DrawTest(VkCommandBuffer cmd);
		void DrawGeometry(VkCommandBuffer cmd);
		void EndRendering(VkCommandBuffer cmd);
		void* GetMappedDataFromAllocation(VmaAllocation allocation);
		void DrawMesh(VkCommandBuffer cmd, DrawData drawData) const;
		
		void DestroyShader(VulkanShader& vk_shader) const;
		void DestroyMesh(VulkanMesh& vk_mesh) const;
		void DestroyBuffer(VulkanBuffer& vk_buffer) const;
		bool m_resizeRequest = false;
		void ResizeSwapchain();
		void UpdateDescriptorSet(GpuDataDescriptor dataDescriptor, VkDescriptorSetLayout setlayout);
	private:
		CleanupQueue m_cleanupQueue;
		VkInstance m_instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
		VkPhysicalDevice m_physDevice = VK_NULL_HANDLE;
		VkDevice m_device = VK_NULL_HANDLE;
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;
		Swapchain m_swapchain;
		VmaAllocator m_allocator = VK_NULL_HANDLE;
		SDL_Window* m_sdl_window = nullptr;

		
		void InitVulkan(SDL_Window* sdl_window);
		void InitSwapchain();
		void InitCommands();
		void InitSyncStructs();
		void InitDescriptors();
		FrameData& GetCurrentFrame();
		FrameData& GetFrameByIndex(size_t index);
		void Cleanup();

		
		std::vector<FrameData> m_frames = {};
		VulkanImage m_drawImage = {};
		VulkanImage m_depthImage= {};
		
		VkQueue m_graphicsQueue = {};
		uint32_t m_graphicsQueueFamily;

		VkQueue m_computeQueue= {};
		uint32_t m_computeQueueFamily;

		VkQueue m_transferQueue = {};
		uint32_t m_transferQueueFamily;

		VkQueue m_presentQueue = {};
		uint32_t m_presentQueueFamily;

		size_t m_currentFrame = {};
		
		DescriptorAllocator m_descriptorAllocator = {};
		VkDescriptorSet _drawImageDescriptors = VK_NULL_HANDLE;
		VkDescriptorSetLayout _drawImageDescriptorLayout = VK_NULL_HANDLE;
		
		VkDescriptorSet _sceneDataDescriptors = VK_NULL_HANDLE;
		//VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;
		VulkanBuffer gpuSceneDataBuffer = {};
		
		//test stuff:
		VkPipeline _gradientPipeline = VK_NULL_HANDLE;
		VkPipelineLayout _gradientPipelineLayout = VK_NULL_HANDLE;
		void init_pipelines();
		void init_background_pipelines();
		
	};
	struct RenderStepData
	{
		VulkanShader* shaderOverride = nullptr;
		VulkanImage* renderTarget = nullptr;
		VulkanImage* depthTarget = nullptr;
		bool randomBool = true;
		VkImageLayout srcImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout dstImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		
		VkRenderingInfo renderingInfo = {};
		VkRenderingAttachmentInfo colorAttachment = {};
		VkRenderingAttachmentInfo depthAttachment = {};
	};
	struct RenderStep
	{
		RenderStepData data;
		std::string m_name;
		RenderStep() = default;
		RenderStep(std::string name, std::function<bool(VkCommandBuffer, RenderStepData&, FrameData&)> renderPassFunction);
		void ExecuteRenderPass(VkCommandBuffer cmd,  FrameData& frameData);
	private:
		std::function<bool(VkCommandBuffer, RenderStepData&,  FrameData&)> m_renderPassFunction;
	};

}

