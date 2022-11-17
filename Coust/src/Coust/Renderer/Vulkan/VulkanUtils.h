#pragma once

#include <volk.h>
#include "vk_mem_alloc.h"

#include <vector>

namespace Coust
{
	namespace VK
	{
		extern VkDevice g_Device;
		extern VkInstance g_Instance;
		extern VmaAllocator g_VmaAlloc;
		extern VkQueue g_GraphicsQueue;
		extern VkPhysicalDevice g_PhysicalDevice;
		extern VkPhysicalDeviceProperties* g_pPhysicalDevProps;

		namespace Utils
		{
#ifndef COUST_FULL_RELEASE
			inline VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
				VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
				VkDebugUtilsMessageTypeFlagsEXT messageType,
				const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
				void* pUserData)
			{
				COUST_CORE_ERROR(pCallbackData->pMessage);
				return VK_FALSE;
			}


			inline VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo()
			{
				VkDebugUtilsMessengerCreateInfoEXT info
				{
					.sType              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
					.messageSeverity    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
					.messageType        = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
					.pfnUserCallback    = DebugCallback,
				};
				return info;
			}
#endif
		}
	}
}
