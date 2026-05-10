#pragma once
#include <functional>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <unordered_map>
#include <vector>
#include "Swapchain.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include "DescriptorLayoutBuilder.h"
#include "../Resources/VulkanResource.h"
#include "VulkanImage.h"
#include "../Gui/GuiBackend.h"
#include "VulkanMesh.h"
#include <glm/glm.hpp>
#include "DrawData.h"
namespace gns::rendering 
{
	struct VulkanShader;
	struct VulkanTexture;

	struct VulkanDefaultTextureHandles
	{
		Handle white;
		Handle grey;
		Handle black;
		Handle errorCheckerboard;
	};
	
	struct CleanupQueue
	{
	private:
		std::deque<std::function<void()>> m_queue;
	public:
		void Push(std::function<void()>&& func);
		void Flush();	
	};
	struct FrameData {
		FrameData() = default;
		FrameData(const FrameData& other)
			: _commandPool(other._commandPool),
			  _mainCommandBuffer(other._mainCommandBuffer),
			  _swapchainSemaphore(other._swapchainSemaphore),
			  _renderSemaphore(other._renderSemaphore),
			  _renderFence(other._renderFence),
			  _swapchain(other._swapchain),
			  _swapchainImageIndex(other._swapchainImageIndex),
			  _sceneDataDescriptors(other._sceneDataDescriptors),
			  _sceneDataBufferUpdated(other._sceneDataBufferUpdated)
		{}

		FrameData& operator=(const FrameData& other)
		{
			if (this != &other)
			{
				_commandPool = other._commandPool;
				_mainCommandBuffer = other._mainCommandBuffer;
				_swapchainSemaphore = other._swapchainSemaphore;
				_renderSemaphore = other._renderSemaphore;
				_renderFence = other._renderFence;
				_swapchain = other._swapchain;
				_swapchainImageIndex = other._swapchainImageIndex;
				_sceneDataDescriptors = other._sceneDataDescriptors;
				_sceneDataBufferUpdated = other._sceneDataBufferUpdated;
			}
			return *this;
		}

		VkCommandPool _commandPool;
		VkCommandBuffer _mainCommandBuffer;
		
		VkSemaphore _swapchainSemaphore; 
		VkSemaphore _renderSemaphore;
		VkFence _renderFence;
		
		CleanupQueue _cleanupQueue;
		
		Swapchain* _swapchain;
		uint32_t _swapchainImageIndex;
		DescriptorAllocatorGrowable _frameDescriptors;
		VulkanBuffer _gpuSceneDataBuffer;
		std::unordered_map<VkDescriptorSetLayout, VkDescriptorSet> _sceneDataDescriptors;
		bool _sceneDataBufferUpdated = false;
	};
	class Device
	{
		struct ComputePushConstants {
			glm::vec4 data1;
			glm::vec4 data2;
			glm::vec4 data3;
			glm::vec4 data4;
		};
		
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
		const VulkanDefaultTextureHandles& GetDefaultTextures() const { return m_defaultTextures; }
		VkDescriptorSetLayout GetTextureDescriptorLayout() const { return _textureDescriptorLayout; }
		uint64_t GetRenderTargetDescriptor() const { return reinterpret_cast<uint64_t>(_renderTargetDescriptor); }
		VkExtent2D GetRenderExtent() const { return m_renderExtent; }
		void SetRenderExtent(VkExtent2D extent);
		void ApplyRenderTargetResize();
		template <DerivedFromVulkanResource Resource_T, typename... Args>
		Resource_T* CreateResource(Args&& ... args)
		{
			return m_resourceRegistry.Create<Resource_T>(this, std::forward<Args>(args)...);
		}

		template <DerivedFromVulkanResource Resource_T>
		Resource_T* GetResource(const Handle handle) const
		{
			return m_resourceRegistry.Get<Resource_T>(handle);
		}

		VkFence m_immediateFence = VK_NULL_HANDLE;
		VkCommandBuffer m_immediateCommandBuffer = VK_NULL_HANDLE;
		VkCommandPool m_immediateCommandPool = VK_NULL_HANDLE;
		
		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
		bool BeginFrame(
			VkCommandBuffer& cmd, uint32_t& swapchainImageIndex, VkExtent2D& extent, FrameData& data);
		void DrawFrame(VkCommandBuffer& cmd, uint32_t& swapchainImageIndex, VkExtent2D& extent , FrameData& data);
		void EndFrame(VkCommandBuffer& cmd, uint32_t& swapchainImageIndex, VkExtent2D& extent , FrameData& data);
		VulkanImage* GetRenderTarget() { return &m_drawImage; }
		VulkanImage* GetDepthTarget() { return &m_depthImage; }

		void DrawBackground(VkCommandBuffer cmd);
		void DrawGeometry(VkCommandBuffer cmd);
		void EndRendering(VkCommandBuffer cmd);
		void TransitionDrawImage(VkCommandBuffer cmd, VkImageLayout newLayout);
		void TransitionDepthImage(VkCommandBuffer cmd, VkImageLayout newLayout);
		void* GetMappedDataFromAllocation(VmaAllocation allocation);
		void DrawMesh(
			VkCommandBuffer cmd,
			DrawData drawData,
			const GpuDataDescriptor* sceneDataDescriptor);
		
		void DestroyShader(VulkanShader& vk_shader) const;
		void DestroyMesh(VulkanMesh& vk_mesh) const;
		void DestroyBuffer(VulkanBuffer& vk_buffer) const;
		void CreateTextureDescriptor(VulkanTexture& texture);
		bool m_resizeRequest = false;
		void ResizeSwapchain();
		VkDescriptorSet UpdateDescriptorSet(GpuDataDescriptor dataDescriptor, VkDescriptorSetLayout setlayout);
	private:
		gns::VulkanResourceRegistry m_resourceRegistry;
		CleanupQueue m_cleanupQueue;
		VkInstance m_instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
		VkPhysicalDevice m_physDevice = VK_NULL_HANDLE;
		VkDevice m_device = VK_NULL_HANDLE;
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;
		Swapchain m_swapchain;
		VmaAllocator m_allocator = VK_NULL_HANDLE;
		SDL_Window* m_sdl_window = nullptr;
		VkExtent2D m_renderExtent = { 1, 1 };
		bool m_useCustomRenderExtent = false;
		bool m_renderTargetResizeRequest = false;

		
		void InitVulkan(SDL_Window* sdl_window);
		void InitSwapchain();
		void InitCommands();
		void InitSyncStructs();
		void InitDescriptors();
		void InitDefaultTextures();
		void CreateDrawTargets(VkExtent2D extent);
		void ResizeDrawTargets(VkExtent2D extent);
		void UpdateDrawImageDescriptor();
		void UpdateRenderTargetDescriptor();
		Handle CreateDefaultTexture(
			const void* data,
			VkExtent3D size,
			VkFilter samplerFilter,
			VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
		FrameData& GetCurrentFrame();
		FrameData& GetFrameByIndex(size_t index);
		void Cleanup();

		
		std::vector<FrameData> m_frames = {};
		VulkanImage m_drawImage = {};
		VulkanImage m_depthImage= {};
		VkImageLayout m_drawImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout m_depthImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		
		VkQueue m_graphicsQueue = {};
		uint32_t m_graphicsQueueFamily;

		VkQueue m_computeQueue= {};
		uint32_t m_computeQueueFamily;

		VkQueue m_transferQueue = {};
		uint32_t m_transferQueueFamily;

		VkQueue m_presentQueue = {};
		uint32_t m_presentQueueFamily;

		size_t m_currentFrame = {};
		
		DescriptorAllocatorGrowable m_descriptorAllocator = {};
		VkDescriptorSet _drawImageDescriptors = VK_NULL_HANDLE;
		VkDescriptorSet _renderTargetDescriptor = VK_NULL_HANDLE;
		VkDescriptorSetLayout _drawImageDescriptorLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout _textureDescriptorLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSetLayout> m_backgroundDescriptorSetLayouts;
		VkSampler _renderTargetSampler = VK_NULL_HANDLE;
		VulkanDefaultTextureHandles m_defaultTextures = {};
		
		VkPipeline m_backgroundPipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_backgroundPipelineLayout = VK_NULL_HANDLE;
		void InitBackgroundResources();
		void CreateBackgroundPipeline();
		
	};
}

