#pragma once
#include "../../Log/Logger.h"
#include <iostream>
#include <vulkan/vulkan.h>

#ifdef _DEBUG
#define VK_LOG
#endif

#ifdef VK_LOG

#define LOG_VK_VERBOSE(msg) \
	std::cout << AQUA <<"[VK_Verbose]: "<< DEFAULT << msg <<" "<<\
	GRAY << FILE_NAME << "(" << __LINE__ << ")" << DEFAULT << "\n"

#define LOG_VK_INFO(msg) \
	std::cout << BLUE <<"[VK_Info]: "<< DEFAULT << msg <<" "<<\
	GRAY << FILE_NAME << "(" << __LINE__ << ")" << DEFAULT << "\n"

#define LOG_VK_WARNING(msg) \
    std::cout << YELLOW << "[VK_Warning]: " << DEFAULT << msg <<" "<<\
    GRAY << FILE_NAME << "(" << __LINE__ << ")" << DEFAULT << "\n"

#define LOG_VK_ERROR(msg) \
    std::cout << RED << "[VK_Error]: " << DEFAULT << msg <<" "<<\
    GRAY << FILE_NAME << "(" << __LINE__ << ")" << DEFAULT << "\n"

#define _VK_CHECK(result, msg)\
    if(result != VK_SUCCESS)LOG_VK_ERROR(msg << " Result: " << result)

//-----------------
#else

#define LOG_VK_VERBOSE(msg)
#define LOG_VK_INFO(msg)
#define LOG_VK_WARNING(msg)
#define LOG_VK_ERROR(msg)
#define _VK_CHECK(result, msg) result

#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(  // NOLINT(clang-diagnostic-unused-function)
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
		LOG_VK_VERBOSE(pCallbackData->pMessage);
	}

	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
		LOG_VK_INFO(pCallbackData->pMessage);
	}

	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		LOG_VK_WARNING(pCallbackData->pMessage);
	}

	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		LOG_VK_ERROR(pCallbackData->pMessage);
	}

	return VK_FALSE;
}