#pragma once
#include <functional>
#include <queue>
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
		
		Swapchain* _swapchain;
		uint32_t _swapchainImageIndex;
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

		VkFence m_immediateFence;
		VkCommandBuffer m_immediateCommandBuffer;
		VkCommandPool m_immediateCommandPool;
		
		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
		void BeginFrame(
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
		void InitDescriptors();
		FrameData& GetCurrentFrame();
		FrameData& GetFrameByIndex(size_t index);
		void Cleanup();

		
		std::vector<FrameData> m_frames;
		VulkanImage m_drawImage;
		VulkanImage m_depthImage;
		
		VkQueue m_graphicsQueue;
		uint32_t m_graphicsQueueFamily;

		VkQueue m_computeQueue;
		uint32_t m_computeQueueFamily;

		VkQueue m_transferQueue;
		uint32_t m_transferQueueFamily;

		VkQueue m_presentQueue;
		uint32_t m_presentQueueFamily;

		size_t m_currentFrame;
		
		DescriptorAllocator m_descriptorAllocator;
		VkDescriptorSet _drawImageDescriptors;
		VkDescriptorSetLayout _drawImageDescriptorLayout;
		
		
		//test stuff:
		VkPipeline _gradientPipeline;
		VkPipelineLayout _gradientPipelineLayout;
		void init_pipelines();
		void init_background_pipelines();
		
		VkPipelineLayout _trianglePipelineLayout;
		VkPipeline _trianglePipeline;

		void init_triangle_pipeline();
		
		//VkPipelineLayout _meshPipelineLayout;
		//VkPipeline _meshPipeline;

		VulkanMesh rectangle;

		void init_mesh_pipeline();
		void init_mesh_data();
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

