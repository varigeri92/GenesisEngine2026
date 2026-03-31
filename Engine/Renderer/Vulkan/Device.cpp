#include "gnspch.h"
#include "Device.h"
#include "vulkan_log.h"
#include <VkBootstrap.h>
#include "vkutils.h"
#include "Pipelines.h"
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include "PipelineBuilder.h"
#include "../Shader.h"
#include "../../Utils/Path.h"
#include "../Resources/VulkanShader.h"

constexpr unsigned int FRAME_OVERLAP = 3;
constexpr bool _useValidationLayers = true;


gns::rendering::RenderStep::RenderStep(std::string name, std::function<bool(VkCommandBuffer, RenderStepData&,  FrameData&)> renderPassFunction)
{
	m_name = name;
	m_renderPassFunction = std::move(renderPassFunction);
}

void gns::rendering::RenderStep::ExecuteRenderPass(VkCommandBuffer cmd,  FrameData& frameData)
{
	if (!m_renderPassFunction(cmd, this->data, frameData))
	{
		LOG_ERROR("Error While Executing RenderPass - '" + m_name + "'");	
	}
}

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

gns::rendering::Device::Device() : m_cleanupQueue{}
{
	m_currentFrame = 0;
	m_frames.reserve(FRAME_OVERLAP);
	for (int i = 0; i < FRAME_OVERLAP; ++i)
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
	auto inst_ret = builder.request_validation_layers(_useValidationLayers).set_debug_callback(VulkanDebugCallback)
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
	m_drawImage = {drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT};
	
	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	
	VkImageCreateInfo rimg_info = utils::ImageCreateInfo(m_drawImage.imageFormat, drawImageUsages, drawImageExtent);

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo rimg_allocinfo = {};
	rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(m_allocator, &rimg_info, &rimg_allocinfo, &m_drawImage.image, &m_drawImage.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo rview_info = utils::ImageViewCreateInfo(m_drawImage.imageFormat, m_drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(m_device, &rview_info, nullptr, &m_drawImage.imageView));

	//add to deletion queues
	m_cleanupQueue.Push([this]() {
		vkDestroyImageView(m_device, m_drawImage.imageView, nullptr);
		vmaDestroyImage(m_allocator, m_drawImage.image, m_drawImage.allocation);
	});
	
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
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
	};

	m_descriptorAllocator.InitPool(m_device, 10, sizes);

	//make the descriptor set layout for our compute draw
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		_drawImageDescriptorLayout = builder.Build(m_device, VK_SHADER_STAGE_COMPUTE_BIT);
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
		vkDestroyDescriptorSetLayout(m_device, _drawImageDescriptorLayout, nullptr);
	});
	
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

void gns::rendering::Device::BeginFrame(
	VkCommandBuffer& cmd, uint32_t& swapchainImageIndex, VkExtent2D& extent, FrameData& data)
{
	VK_CHECK(vkWaitForFences(m_device, 1, &GetCurrentFrame()._renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(m_device, 1, &GetCurrentFrame()._renderFence));
	GetCurrentFrame()._cleanupQueue.Flush();
	
	VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain.GetSwapchain(), 
		1000000000, GetCurrentFrame()._swapchainSemaphore, nullptr, &swapchainImageIndex));
	
	extent = {m_drawImage.imageExtent.width, m_drawImage.imageExtent.height};
	data = GetCurrentFrame();
	cmd = data._mainCommandBuffer;
	data._swapchainImageIndex=swapchainImageIndex;
	data._swapchain = &m_swapchain;
	VK_CHECK(vkResetCommandBuffer(cmd, 0));
	VkCommandBufferBeginInfo cmdBeginInfo = utils::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
}

void gns::rendering::Device::ExecuteRenderPasses(VkCommandBuffer& cmd,  FrameData& frameData)
{
	for (auto& renderPass : renderPasses)
	{
		renderPass.ExecuteRenderPass(cmd, frameData);
	}
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

	VK_CHECK(vkQueuePresentKHR(m_graphicsQueue, &presentInfo));
	
	m_currentFrame++;
}

gns::rendering::RenderStep& gns::rendering::Device::CreateRenderPass(std::string name,
			std::function<bool(VkCommandBuffer, RenderStepData&,  FrameData&)> renderPassFunction)
{
	renderPasses.emplace_back(name, renderPassFunction);
	return renderPasses[renderPasses.size()-1];
}

void gns::rendering::Device::Cleanup()
{
	vkDeviceWaitIdle(m_device);
	for (size_t i = 0; i < FRAME_OVERLAP; i++) 
	{
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
	
	/* 
	//make a clear-color from frame number. This will flash with a 120 frame period.
	VkClearColorValue clearValue;
	float flash = std::abs(std::sin(m_currentFrame / 120.f));
	clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

	VkImageSubresourceRange clearRange = utils::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

	//clear image
	vkCmdClearColorImage(cmd, m_drawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
	*/
}

void gns::rendering::Device::init_pipelines()
{
	init_background_pipelines();
	init_triangle_pipeline();
	init_mesh_pipeline();
	//init_mesh_data();
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

void gns::rendering::Device::init_triangle_pipeline()
{
	std::string fragmentShaderPath = gns::path::InResourcesDirectory(R"(Shaders\default.frag)").string();
	std::string vertexShaderPath = gns::path::InResourcesDirectory(R"(Shaders\default.vert)").string();
	
	VkPipelineLayoutCreateInfo pipeline_layout_info = utils::PipelineLayoutCreateInfo();
	VK_CHECK(vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &_trianglePipelineLayout));
	
	Shader* shader = Object::Create<Shader>(vertexShaderPath, fragmentShaderPath, "default_shader");
	PipelineBuilder builder{this};
	builder.m_pipelineLayout = _trianglePipelineLayout;
	builder.SetShaders(*shader);
	builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	//filled triangles
	builder.SetPolygonMode(VK_POLYGON_MODE_FILL);
	//no backface culling
	builder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	//no multisampling
	builder.SetMultisampling();
	//no blending
	builder.DisableBlending();
	//no depth testing
	builder.DisableDepthTest();
	
	builder.SetColorAttachmentFormat(m_drawImage.imageFormat);
	builder.SetDepthFormat(VK_FORMAT_UNDEFINED);
	
	_trianglePipeline = builder.BuildPipeline(m_device);
	
	m_cleanupQueue.Push([&]() {
		vkDestroyPipelineLayout(m_device, _trianglePipelineLayout, nullptr);
		vkDestroyPipeline(m_device, _trianglePipeline, nullptr);
	});
}

void gns::rendering::Device::init_mesh_pipeline()
{
	/*
	 
	std::string fragmentShaderPath = gns::path::InResourcesDirectory(R"(Shaders\default.frag)").string();
	std::string vertexShaderPath = gns::path::InResourcesDirectory(R"(Shaders\mesh.vert)").string();
	
	VkPushConstantRange bufferRange{};
	bufferRange.offset = 0;
	bufferRange.size = sizeof(GPUDrawPushConstants);
	bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	
	VkPipelineLayoutCreateInfo pipeline_layout_info = utils::PipelineLayoutCreateInfo();
	pipeline_layout_info.pPushConstantRanges = &bufferRange;
	pipeline_layout_info.pushConstantRangeCount = 1;
	VK_CHECK(vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &_meshPipelineLayout));
	
	Shader* shader = Object::Create<Shader>(vertexShaderPath, fragmentShaderPath, "default_mesh_shader");
	
	PipelineBuilder builder{this};
	builder.m_pipelineLayout = _meshPipelineLayout;
	builder.SetShaders(*shader);
	builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	//filled triangles
	builder.SetPolygonMode(VK_POLYGON_MODE_FILL);
	//no backface culling
	builder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	//no multisampling
	builder.SetMultisampling();
	//no blending
	builder.DisableBlending();
	//no depth testing
	builder.DisableDepthTest();
	
	builder.SetColorAttachmentFormat(m_drawImage.imageFormat);
	builder.SetDepthFormat(VK_FORMAT_UNDEFINED);
	
	_meshPipeline = builder.BuildPipeline(m_device);
	
	m_cleanupQueue.Push([&]() {
		vkDestroyPipelineLayout(m_device, _meshPipelineLayout, nullptr);
		vkDestroyPipeline(m_device, _meshPipeline, nullptr);
	});
	*/	
}

void gns::rendering::Device::init_mesh_data()
{
	std::array<Vertex,4> rect_vertices;
	rect_vertices[0].position = {0.5,-0.5, 0};
	rect_vertices[1].position = {0.5,0.5, 0};
	rect_vertices[2].position = {-0.5,-0.5, 0};
	rect_vertices[3].position = {-0.5,0.5, 0};

	rect_vertices[0].color = {0,0, 0,1};
	rect_vertices[1].color = { 0.5,0.5,0.5 ,1};
	rect_vertices[2].color = { 1,0, 0,1 };
	rect_vertices[3].color = { 0,1, 0,1 };

	std::array<uint32_t,6> rect_indices;

	rect_indices[0] = 0;
	rect_indices[1] = 1;
	rect_indices[2] = 2;

	rect_indices[3] = 2;
	rect_indices[4] = 1;
	rect_indices[5] = 3;

	//rectangle = VulkanMesh::UploadMesh(*this, m_allocator, rect_indices,rect_vertices);

	//delete the rectangle data on engine shutdown
	m_cleanupQueue.Push([&](){
		rectangle.indexBuffer.reset();
		rectangle.vertexBuffer.reset();
	});
}

void gns::rendering::Device::DrawMesh(
VkCommandBuffer cmd, DrawData draw_data) const
{
	VkPipeline pipeline = draw_data.vkShader.GetPipeline();
	VkPipelineLayout layout = draw_data.vkShader.GetPipelineLayout();
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	GPUDrawPushConstants push_constants;
	push_constants.worldMatrix = draw_data.transform;
	push_constants.vertexBuffer = draw_data.vk_vertexBufferAddress;

	vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
	vkCmdBindIndexBuffer(cmd, draw_data.vk_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(cmd, static_cast<uint32_t>(draw_data.Count), 1, draw_data.StartIndex, 0, 0);
}

void gns::rendering::Device::EndRendering(VkCommandBuffer cmd)
{
	vkCmdEndRendering(cmd);
}

void gns::rendering::Device::DrawGeometry(VkCommandBuffer cmd)
{
	//begin a render pass  connected to our draw image
	VkRenderingAttachmentInfo colorAttachment = utils::AttachmentInfo(m_drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	
	VkRenderingInfo renderInfo = utils::RenderingInfo(m_swapchain.GetExtent(), &colorAttachment, nullptr);
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
	
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _trianglePipeline);
	//launch a draw command to draw 3 vertices
	vkCmdDraw(cmd, 3, 1, 0, 0);
}

void* gns::rendering::Device::GetMappedDataFromAllocation(VmaAllocation allocation)
{
	return allocation->GetMappedData();
}
