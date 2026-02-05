#include "gnspch.h"
#include "Device.h"
#include "vulkan_log.h"
#include <VkBootstrap.h>

bool _useValidationLayers = true;
gns::rendering::Device::Device()
{
	m_frames.reserve(FRAME_OVERLAP);
	for (int i = 0; i < FRAME_OVERLAP; ++i)
	{
		m_frames.emplace_back();
	}
}
gns::rendering::Device::~Device()
{
	LOG_INFO("Device Puff!");
	Cleanup();
}
void gns::rendering::Device::Create(SDL_Window* sdl_window)
{
	m_sdl_window = sdl_window;
	

	InitVulkan(sdl_window);
	InitSwapchain();
	InitCommands();
	InitSyncStructs();
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
}

void gns::rendering::Device::InitCommands()
{
	//create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo commandPoolInfo = {};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.pNext = nullptr;
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolInfo.queueFamilyIndex = m_graphicsQueueFamily;

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		_VK_CHECK(vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &m_frames[i]._commandPool), "CommandPool Create failed!");
		// allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = {};
		cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdAllocInfo.pNext = nullptr;
		cmdAllocInfo.commandPool = m_frames[i]._commandPool;
		cmdAllocInfo.commandBufferCount = 1;
		cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		_VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_frames[i]._mainCommandBuffer), "CommandPool allocation Failed!");
	}
}

void gns::rendering::Device::InitSyncStructs()
{
}

void gns::rendering::Device::Cleanup()
{
	m_swapchain.DestroySwapchain();
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyDevice(m_device, nullptr);
	vkb::destroy_debug_utils_messenger(m_instance, m_debugMessenger);
	vkDestroyInstance(m_instance, nullptr);
}
