#include "gnspch.h"
#include "Device.h"
#include "vulkan_log.h"
#include <VkBootstrap.h>
#include "vkutils.h"
#include "Pipelines.h"
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <array>
#include <utility>
#include <glm/gtc/packing.hpp>

#include "PipelineBuilder.h"
#include "../Shader.h"
#include "../../Scene/Scene.h"
#include "../../Utils/Path.h"
#include "../Resources/VulkanShader.h"
#include "../Resources/VulkanTexture.h"

constexpr unsigned int FRAME_OVERLAP = 3;
constexpr bool useValidationLayers = true;


void gns::rendering::CleanupQueue::Push(std::function<void()>&& func)
{
	m_queue.push_back(func);
}

void gns::rendering::CleanupQueue::Flush()
{
	for (auto it = m_queue.rbegin(); it != m_queue.rend(); it++) {
		(*it)();
	}
	m_queue.clear();
}

gns::rendering::Device::Device() : 
m_graphicsQueueFamily(0), m_computeQueueFamily(0), m_transferQueueFamily(0),m_presentQueueFamily(0)
{
	m_currentFrame = 0;
	m_frames.reserve(FRAME_OVERLAP);
	for (unsigned int i = 0; i < FRAME_OVERLAP; ++i)
	{
		m_frames.emplace_back();
	}
}

gns::rendering::Device::~Device()
{
	Cleanup();
}
void gns::rendering::Device::Create(SDL_Window* sdl_window)
{
	m_sdl_window = sdl_window;
	

	InitVulkan(sdl_window);
	InitSwapchain();
	InitCommands();
	InitSyncStructs();
	InitDescriptors();
	InitDefaultTextures();
	
	//init pipeline for test:
	init_pipelines();
}

void gns::rendering::Device::WaitForIdle()
{
	vkDeviceWaitIdle(m_device);
}

void gns::rendering::Device::InitVulkan(SDL_Window* sdl_window)
{
	vkb::InstanceBuilder builder;
	auto inst_ret = builder.request_validation_layers(useValidationLayers).set_debug_callback(VulkanDebugCallback)
		.set_debug_messenger_severity(
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		.set_app_name("Genesis Engine")
		.require_api_version(1, 3, 0)
		.build();

	vkb::Instance vkb_inst = inst_ret.value();

	m_instance = vkb_inst.instance;
	m_debugMessenger = vkb_inst.debug_messenger;

	SDL_Vulkan_CreateSurface(sdl_window, m_instance, &m_surface);

	//vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features.dynamicRendering = true;
	features.synchronization2 = true;

	//vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;


	//use vkbootstrap to select a gpu. 
	//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features)
		.set_required_features_12(features12)
		.set_surface(m_surface)
		.select()
		.value();


	//create the final vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();
	
	// Get the VkDevice handle used in the rest of a vulkan application
	m_device = vkbDevice.device;
	m_physDevice = physicalDevice.physical_device;

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = m_physDevice;
	allocatorInfo.device = m_device;
	allocatorInfo.instance = m_instance;
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	vmaCreateAllocator(&allocatorInfo, &m_allocator);

	m_cleanupQueue.Push([&]() {
		vmaDestroyAllocator(m_allocator);
	});

	m_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	m_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	m_computeQueue = vkbDevice.get_queue(vkb::QueueType::compute).value();
	m_computeQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::compute).value();

	m_transferQueue = vkbDevice.get_queue(vkb::QueueType::transfer).value();
	m_transferQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();

	m_presentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();
	m_presentQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::present).value();
}

void gns::rendering::Device::InitSwapchain()
{
	int w, h;
	SDL_GetWindowSize(m_sdl_window, &w, &h);
	VkExtent2D _extent{ static_cast<uint32_t>(w),static_cast<uint32_t>(h) };
	m_swapchain.CreateSwapchain(this, _extent);
	
	VkExtent3D drawImageExtent = {
	static_cast<uint32_t>(w),static_cast<uint32_t>(h), 1
	};
	m_drawImage = VulkanImage(drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT);
	m_drawImage.m_device = this;
	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	m_drawImage.CreateImage(drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages, false);
	
	m_depthImage = VulkanImage(drawImageExtent, VK_FORMAT_D32_SFLOAT);
	m_depthImage.m_device = this;
	VkImageUsageFlags depthImageUsages{};
	depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	m_depthImage.CreateImage(drawImageExtent, VK_FORMAT_D32_SFLOAT, depthImageUsages, false);
	
	for (size_t i = 0; i < FRAME_OVERLAP; i++) {
		std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = { 
			{.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .ratio = 3 },
			{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .ratio = 3 },
			{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .ratio = 3 },
			{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .ratio = 4 },
		};

		m_frames[i]._frameDescriptors = DescriptorAllocatorGrowable{};
		m_frames[i]._frameDescriptors.Init(m_device, 1000, frame_sizes);
	
		m_cleanupQueue.Push([&, i]() {
			m_frames[i]._frameDescriptors.DestroyPools(m_device);
		});
	}
	
	
	//add to deletion queues
	m_cleanupQueue.Push([this]() {
		vkDestroyImageView(m_device, m_drawImage.imageView, nullptr);
		vmaDestroyImage(m_allocator, m_drawImage.image, m_drawImage.allocation);
		
		vkDestroyImageView(m_device, m_depthImage.imageView, nullptr);
		vmaDestroyImage(m_allocator, m_depthImage.image, m_depthImage.allocation);
	});
	
}

void gns::rendering::Device::ResizeSwapchain()
{
	vkDeviceWaitIdle(m_device);
	m_swapchain.ResizeSwapchain(m_resizeRequest, m_sdl_window);
}

void gns::rendering::Device::UpdateDescriptorSet(GpuDataDescriptor dataDescriptor, VkDescriptorSetLayout setlayout)
{
	FrameData& frame = GetCurrentFrame();
	void* mappedBufferData = frame._gpuSceneDataBuffer.allocation->GetMappedData();
	memcpy(mappedBufferData, dataDescriptor.data, dataDescriptor.size);
	
	frame._sceneDataDescriptors = frame._frameDescriptors.Allocate(m_device, setlayout);
	DescriptorWriter writer;
	writer.WriteBuffer(0, frame._gpuSceneDataBuffer.buffer, dataDescriptor.size, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.UpdateSet(m_device, frame._sceneDataDescriptors);
}

void gns::rendering::Device::InitCommands()
{
	//create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo commandPoolInfo = 
		utils::CommandPoolCreateInfo(m_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	for (int i = 0; i < FRAME_OVERLAP; i++) {
		_VK_CHECK(vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &m_frames[i]._commandPool),
			"CommandPool Create failed!");
		VkCommandBufferAllocateInfo cmdAllocInfo = 
			utils::CommandBufferAllocateInfo(m_frames[i]._commandPool);
		_VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_frames[i]._mainCommandBuffer),
			"CommandPool allocation Failed!");
	}
	
	
	VK_CHECK(vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &m_immediateCommandPool));

	// allocate the command buffer for immediate submits
	VkCommandBufferAllocateInfo cmdAllocInfo = utils::CommandBufferAllocateInfo(m_immediateCommandPool, 1);

	VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_immediateCommandBuffer));

	m_cleanupQueue.Push([=]() { 
	vkDestroyCommandPool(m_device, m_immediateCommandPool, nullptr);
	});
	
}

void gns::rendering::Device::InitSyncStructs()
{
	VkFenceCreateInfo fenceCreateInfo = utils::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = utils::SemaphoreCreateInfo();

	for (size_t i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_frames[i]._renderFence));
		VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frames[i]._swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frames[i]._renderSemaphore));
	}
	
	VK_CHECK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_immediateFence));
	m_cleanupQueue.Push([=]() 
		{ vkDestroyFence(m_device, m_immediateFence, nullptr); });
}

void gns::rendering::Device::InitDescriptors()
{
	//create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<DescriptorAllocator::PoolSizeRatio> sizes =
	{
		{.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .ratio = 1 },
		{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .ratio = 8 }
	};

	m_descriptorAllocator.InitPool(m_device, 32, sizes);

	//make the descriptor set layout for our compute draw
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		_drawImageDescriptorLayout = builder.Build(m_device, VK_SHADER_STAGE_COMPUTE_BIT);
	}
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		_textureDescriptorLayout = builder.Build(m_device, VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	//allocate a descriptor set for our draw image
	_drawImageDescriptors = m_descriptorAllocator.Allocate(m_device,_drawImageDescriptorLayout);	
	
	VkDescriptorImageInfo imgInfo{};
	imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imgInfo.imageView = m_drawImage.imageView;
	
	VkWriteDescriptorSet drawImageWrite = {};
	drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	drawImageWrite.pNext = nullptr;
	
	drawImageWrite.dstBinding = 0;
	drawImageWrite.dstSet = _drawImageDescriptors;
	drawImageWrite.descriptorCount = 1;
	drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	drawImageWrite.pImageInfo = &imgInfo;

	vkUpdateDescriptorSets(m_device, 1, &drawImageWrite, 0, nullptr);

	//make sure both the descriptor allocator and the new layout get cleaned up properly
	m_cleanupQueue.Push([&]() {
		m_descriptorAllocator.DestroyPool(m_device);
		if (_drawImageDescriptorLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(m_device, _drawImageDescriptorLayout, nullptr);
			_drawImageDescriptorLayout = VK_NULL_HANDLE;
		}
		if (_textureDescriptorLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(m_device, _textureDescriptorLayout, nullptr);
			_textureDescriptorLayout = VK_NULL_HANDLE;
		}
	});
	
}

void gns::rendering::Device::InitDefaultTextures()
{
	const uint32_t white = glm::packUnorm4x8(glm::vec4(1.f, 1.f, 1.f, 1.f));
	const uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1.f));
	const uint32_t black = glm::packUnorm4x8(glm::vec4(0.f, 0.f, 0.f, 1.f));
	const uint32_t magenta = glm::packUnorm4x8(glm::vec4(1.f, 0.f, 1.f, 1.f));

	m_defaultTextures.white = CreateDefaultTexture(
		&white, VkExtent3D{1, 1, 1}, VK_FILTER_LINEAR);
	m_defaultTextures.grey = CreateDefaultTexture(
		&grey, VkExtent3D{1, 1, 1}, VK_FILTER_LINEAR);
	m_defaultTextures.black = CreateDefaultTexture(
		&black, VkExtent3D{1, 1, 1}, VK_FILTER_LINEAR);

	std::array<uint32_t, 16 * 16> pixels = {};
	for (uint32_t y = 0; y < 16; ++y)
	{
		for (uint32_t x = 0; x < 16; ++x)
		{
			pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
		}
	}

	m_defaultTextures.errorCheckerboard = CreateDefaultTexture(
		pixels.data(), VkExtent3D{16, 16, 1}, VK_FILTER_NEAREST);
}

gns::Handle gns::rendering::Device::CreateDefaultTexture(
	const void* data,
	VkExtent3D size,
	VkFilter samplerFilter,
	VkSamplerAddressMode samplerAddressMode)
{
	VulkanTexture* texture = CreateResource<VulkanTexture>();
	if (texture == nullptr)
	{
		LOG_ERROR("[Device]: Failed to create default texture resource.");
		return {};
	}

	VkSamplerCreateInfo samplerInfo = utils::SamplerCreateInfo(samplerFilter, samplerAddressMode);
	VK_CHECK(vkCreateSampler(m_device, &samplerInfo, nullptr, &texture->sampler));

	texture->CreateTexture(
		data,
		size,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		false);
	CreateTextureDescriptor(*texture);
	if (texture->descriptorSet == VK_NULL_HANDLE)
	{
		LOG_ERROR("[Device]: Failed to create descriptor for default texture.");
		LOG_ERROR(std::to_string(texture->GetHandle().Get()));
		return {};
	}

	return texture->GetHandle();
}

gns::rendering::FrameData& gns::rendering::Device::GetCurrentFrame()
{
	size_t currentFrame = m_currentFrame % FRAME_OVERLAP;
	return m_frames[currentFrame];
}

gns::rendering::FrameData& gns::rendering::Device::GetFrameByIndex(size_t index)
{
	return m_frames[index];
}

void gns::rendering::Device::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VK_CHECK(vkResetFences(m_device, 1, &m_immediateFence));
	VK_CHECK(vkResetCommandBuffer(m_immediateCommandBuffer, 0));
	VkCommandBuffer cmd = m_immediateCommandBuffer;
	VkCommandBufferBeginInfo cmdBeginInfo = utils::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
	
	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));
	VkCommandBufferSubmitInfo cmdinfo = utils::CommandBufferSubmitInfo(cmd);
	VkSubmitInfo2 submit = utils::SubmitInfo(&cmdinfo, nullptr, nullptr);
	VK_CHECK(vkQueueSubmit2(m_graphicsQueue, 1, &submit, m_immediateFence));
	VK_CHECK(vkWaitForFences(m_device, 1, &m_immediateFence, true, 9999999999));
}

void gns::rendering::Device::DrawFrame(
	VkCommandBuffer& cmd, uint32_t& swapchainImageIndex, VkExtent2D& extent, FrameData& data)
{
	
}

bool gns::rendering::Device::BeginFrame(
	VkCommandBuffer& cmd, uint32_t& swapchainImageIndex, VkExtent2D& extent, FrameData& data)
{
	VK_CHECK(vkWaitForFences(m_device, 1, &GetCurrentFrame()._renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(m_device, 1, &GetCurrentFrame()._renderFence));
	GetCurrentFrame()._frameDescriptors.ClearPools(m_device);
	GetCurrentFrame()._gpuSceneDataBuffer.reset();
	GetCurrentFrame()._sceneDataDescriptors = VK_NULL_HANDLE;
	GetCurrentFrame()._cleanupQueue.Flush();
	
	VkResult e = vkAcquireNextImageKHR(m_device, m_swapchain.GetSwapchain(), 
		1000000000, GetCurrentFrame()._swapchainSemaphore, nullptr, &swapchainImageIndex);
	if (e == VK_ERROR_OUT_OF_DATE_KHR) {
		m_resizeRequest = true;
		return false;
	}
	extent = {.width = m_drawImage.imageExtent.width, .height = m_drawImage.imageExtent.height};
	data = GetCurrentFrame();
	cmd = data._mainCommandBuffer;
	data._swapchainImageIndex=swapchainImageIndex;
	data._swapchain = &m_swapchain;
	VK_CHECK(vkResetCommandBuffer(cmd, 0));
	VkCommandBufferBeginInfo cmdBeginInfo = utils::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
	
	GetCurrentFrame()._gpuSceneDataBuffer.allocator = m_allocator;
	GetCurrentFrame()._gpuSceneDataBuffer.CreateBuffer(
		sizeof(SceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	
	return true;
}

void gns::rendering::Device::EndFrame(
	VkCommandBuffer& cmd, uint32_t& swapchainImageIndex, VkExtent2D& extent, FrameData& data )
{
	VK_CHECK(vkEndCommandBuffer(cmd));
	
	
	VkCommandBufferSubmitInfo cmdinfo = utils::CommandBufferSubmitInfo(cmd);	
	VkSemaphoreSubmitInfo waitInfo = utils::SemaphoreSubmitInfo(
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,data._swapchainSemaphore);
	VkSemaphoreSubmitInfo signalInfo = utils::SemaphoreSubmitInfo(
		VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, data._renderSemaphore);
	VkSubmitInfo2 submit = utils::SubmitInfo(&cmdinfo,&signalInfo,&waitInfo);	
	VK_CHECK(vkQueueSubmit2(m_graphicsQueue, 1, &submit, data._renderFence));
	
	
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = m_swapchain.GetSwapchain_ptr();
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &GetCurrentFrame()._renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VkResult presentResult = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);
	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
		m_resizeRequest = true;
	}
	m_currentFrame++;
}

void gns::rendering::Device::Cleanup()
{
	vkDeviceWaitIdle(m_device);
	
	m_resourceRegistry.DestroyAll();
	
	for (size_t i = 0; i < FRAME_OVERLAP; i++) 
	{
		m_frames[i]._frameDescriptors.ClearPools(m_device);
		m_frames[i]._gpuSceneDataBuffer.reset();
		m_frames[i]._sceneDataDescriptors = VK_NULL_HANDLE;
		
		vkDestroyFence(m_device, m_frames[i]._renderFence, nullptr);
		vkDestroySemaphore(m_device, m_frames[i]._swapchainSemaphore, nullptr);
		vkDestroySemaphore(m_device, m_frames[i]._renderSemaphore, nullptr);
		
		vkDestroyCommandPool(m_device, m_frames[i]._commandPool, nullptr);
		
		m_frames[i]._cleanupQueue.Flush();
	}
	m_cleanupQueue.Flush();
	m_swapchain.DestroySwapchain();
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyDevice(m_device, nullptr);
	vkb::destroy_debug_utils_messenger(m_instance, m_debugMessenger);
	vkDestroyInstance(m_instance, nullptr);
}

glm::vec4 to01rgba(float r, float g, float b, float a)
{
	return { r / 255 ,g / 255,  b/255,  a /255 };
}
void gns::rendering::Device::DrawTest(VkCommandBuffer cmd)
{
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipeline);
	vkCmdBindDescriptorSets(
		cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0,
		1, &_drawImageDescriptors, 0, nullptr);
	
	ComputePushConstants pc;
	pc.data1 = to01rgba(182, 202, 255, 255);
	pc.data2 = to01rgba(87, 95, 99, 255);
	pc.data2 = to01rgba(0, 0, 0, 255);

	vkCmdPushConstants(cmd, _gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &pc);

	
	vkCmdDispatch(cmd, 
		static_cast<uint32_t>(std::ceil(m_swapchain.GetExtent().width / 16.0)), 
		static_cast<uint32_t>(std::ceil(m_swapchain.GetExtent().height / 16.0)), 
		1);
}

void gns::rendering::Device::init_pipelines()
{
	init_background_pipelines();
}

void gns::rendering::Device::init_background_pipelines()
{
	VkPipelineLayoutCreateInfo computeLayout{};
	computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computeLayout.pNext = nullptr;
	computeLayout.pSetLayouts = &_drawImageDescriptorLayout;
	computeLayout.setLayoutCount = 1;

	VkPushConstantRange pushConstant{};
	pushConstant.offset = 0;
	pushConstant.size = sizeof(ComputePushConstants) ;
	pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	computeLayout.pPushConstantRanges = &pushConstant;
	computeLayout.pushConstantRangeCount = 1;
	
	VK_CHECK(vkCreatePipelineLayout(m_device, &computeLayout, nullptr, &_gradientPipelineLayout));
	
	VkShaderModule computeDrawShader;
	std::string shaderPath = gns::path::InResourcesDirectory(R"(Shaders\sky.comp.spv)").string();
	if (!utils::LoadShaderModule(shaderPath, m_device, &computeDrawShader))
	{
		LOG_ERROR("Error when building the compute shader \n");
	}

	VkPipelineShaderStageCreateInfo stageinfo{};
	stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageinfo.pNext = nullptr;
	stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageinfo.module = computeDrawShader;
	stageinfo.pName = "main";

	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext = nullptr;
	computePipelineCreateInfo.layout = _gradientPipelineLayout;
	computePipelineCreateInfo.stage = stageinfo;
	
	VK_CHECK(vkCreateComputePipelines(m_device,VK_NULL_HANDLE,1,&computePipelineCreateInfo, nullptr, &_gradientPipeline));

	vkDestroyShaderModule(m_device, computeDrawShader, nullptr);

	m_cleanupQueue.Push([&]() {
		vkDestroyPipelineLayout(m_device, _gradientPipelineLayout, nullptr);
		vkDestroyPipeline(m_device, _gradientPipeline, nullptr);
		});
}

void gns::rendering::Device::DrawMesh(VkCommandBuffer cmd, DrawData draw_data)
{
	VkPipeline pipeline = draw_data.vkShader->GetPipeline();
	VkPipelineLayout layout = draw_data.vkShader->GetPipelineLayout();
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	vkCmdBindDescriptorSets(cmd, 
		VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &GetCurrentFrame()._sceneDataDescriptors, 0, nullptr);
	
	GPUDrawPushConstants push_constants;
	push_constants.worldMatrix = draw_data.transform;
	push_constants.vertexBuffer = draw_data.vk_vertexBufferAddress;

	vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
	vkCmdBindIndexBuffer(cmd, draw_data.vk_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(cmd, static_cast<uint32_t>(draw_data.Count), 1, static_cast<uint32_t>(draw_data.StartIndex), 0, 0);
}

void gns::rendering::Device::DestroyShader(VulkanShader& vk_shader) const
{
	if (vk_shader.m_pipelineLayout != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(m_device, vk_shader.m_pipelineLayout, nullptr);
	if (vk_shader.m_pipeline != VK_NULL_HANDLE)
		vkDestroyPipeline(m_device, vk_shader.m_pipeline, nullptr);
	if (vk_shader.m_descriptorSetLayout != VK_NULL_HANDLE)
		vkDestroyDescriptorSetLayout(m_device, vk_shader.m_descriptorSetLayout, nullptr);
	
	vk_shader.m_pipelineLayout = VK_NULL_HANDLE;
	vk_shader.m_pipeline = VK_NULL_HANDLE;
	vk_shader.m_descriptorSetLayout = VK_NULL_HANDLE;
	
}

void gns::rendering::Device::DestroyMesh(VulkanMesh& vk_mesh) const
{
	DestroyBuffer(vk_mesh.indexBuffer);
	DestroyBuffer(vk_mesh.vertexBuffer);
}

void gns::rendering::Device::DestroyBuffer(VulkanBuffer& vk_buffer) const
{
	if (vk_buffer.buffer != VK_NULL_HANDLE)
		vk_buffer.reset();
	vk_buffer.buffer = VK_NULL_HANDLE;
}

void gns::rendering::Device::CreateTextureDescriptor(VulkanTexture& texture)
{
	if (_textureDescriptorLayout == VK_NULL_HANDLE ||
		texture.sampler == VK_NULL_HANDLE ||
		texture.image.imageView == VK_NULL_HANDLE)
	{
		LOG_ERROR("[Device]: Cannot create texture descriptor because texture resources are incomplete.");
		LOG_ERROR(std::to_string(texture.GetHandle().Get()));
		return;
	}

	texture.descriptorSet = m_descriptorAllocator.Allocate(m_device, _textureDescriptorLayout);
	DescriptorWriter writer;
	writer.WriteImage(
		0,
		texture.image.imageView,
		texture.sampler,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.UpdateSet(m_device, texture.descriptorSet);
}

void gns::rendering::Device::EndRendering(VkCommandBuffer cmd)
{
	vkCmdEndRendering(cmd);
}

void gns::rendering::Device::DrawGeometry(VkCommandBuffer cmd)
{
	//begin a render pass  connected to our draw image
	VkRenderingAttachmentInfo colorAttachment = utils::AttachmentInfo(
		m_drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingAttachmentInfo depthAttachment = utils::DepthAttachmentInfo(
		m_depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
	
	VkRenderingInfo renderInfo = utils::RenderingInfo(
		{m_drawImage.imageExtent.width,m_drawImage.imageExtent.height }, &colorAttachment, &depthAttachment);
	vkCmdBeginRendering(cmd, &renderInfo);

	//set dynamic viewport and scissor
	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = static_cast<float>(m_swapchain.GetExtent().width);
	viewport.height = static_cast<float>(m_swapchain.GetExtent().height);
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;
	
	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = m_swapchain.GetExtent().width;
	scissor.extent.height = m_swapchain.GetExtent().height;

	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &scissor);
	/* 
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _trianglePipeline);
	//launch a draw command to draw 3 vertices
	vkCmdDraw(cmd, 3, 1, 0, 0);
	*/
}

void* gns::rendering::Device::GetMappedDataFromAllocation(VmaAllocation allocation)
{
	return allocation->GetMappedData();
}
