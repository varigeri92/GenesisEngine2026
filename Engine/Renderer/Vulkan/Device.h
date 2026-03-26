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

#include <glm/glm.hpp>
namespace gns::rendering 
{
	class VulkanShader;

	struct RenderPassData
	{
		VulkanShader* shaderOverride = nullptr;
		VulkanImage* renderTarget = nullptr;
		bool randomBool = true;
	};
	struct RenderPass
	{
		RenderPassData data;
		std::string m_name;
		RenderPass() = default;
		RenderPass(std::string name, std::function<bool(VkCommandBuffer, RenderPassData&)> renderPassFunction);
		void ExecuteRenderPass(VkCommandBuffer cmd);
	private:
		std::function<bool(VkCommandBuffer, RenderPassData&)> m_renderPassFunction;
	};
	
	class Device
	{
		struct ComputePushConstants {
			glm::vec4 data1;
			glm::vec4 data2;
			glm::vec4 data3;
			glm::vec4 data4;
		};
		
		std::vector<RenderPass> renderPasses;
		
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

	public:
		struct FrameData {
			VkCommandPool _commandPool;
			VkCommandBuffer _mainCommandBuffer;
			
			VkSemaphore _swapchainSemaphore; 
			VkSemaphore _renderSemaphore;
			VkFence _renderFence;
			
			CleanupQueue _cleanupQueue;
		};
		
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
		
		VkFence m_immediateFence;
		VkCommandBuffer m_immediateCommandBuffer;
		VkCommandPool m_immediateCommandPool;
		
		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
		void BeginFrame(VkCommandBuffer& cmd, uint32_t& swapchainImageIndex, VkExtent2D& extent, FrameData& data);
		void DrawFrame(VkCommandBuffer& cmd, uint32_t& swapchainImageIndex, VkExtent2D& extent , FrameData& data);
		void ExecuteRenderPasses(VkCommandBuffer& cmd);
		void EndFrame(VkCommandBuffer& cmd, uint32_t& swapchainImageIndex, VkExtent2D& extent , FrameData& data);
		RenderPass& CreateRenderPass(std::string name, 
			std::function<bool(VkCommandBuffer, RenderPassData&)> renderPassFunction);

		void DrawTest(VkCommandBuffer cmd);
		
	//private:
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
		
		
		//test pipelines:
		VkPipeline _gradientPipeline;
		VkPipelineLayout _gradientPipelineLayout;
		void init_pipelines();
		void init_background_pipelines();
		
		VkPipelineLayout _trianglePipelineLayout;
		VkPipeline _trianglePipeline;

		void init_triangle_pipeline();
		void DrawGeometry(VkCommandBuffer cmd);
		
	};
}

