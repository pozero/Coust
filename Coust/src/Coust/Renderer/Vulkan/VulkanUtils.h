#pragma once

#include <volk.h>
#include "vk_mem_alloc.h"

#include "Coust/Renderer/Vulkan/VulkanStructs.h"
#include "Coust/Renderer/Vulkan/VulkanSwapchain.h"

#include <vector>
#include <optional>

#define VK_CHECK(func)																													\
	do																																	\
	{																																	\
		VkResult err = func;																											\
		if (err != VK_SUCCESS)																											\
		{																																\
			COUST_CORE_ERROR("File {0}, Line{1}, Vulkan Func {2} return {3}", __FILE__, __LINE__, #func, VulkanReturnCodeToStr(err));	\
			return false;																												\
		}																																\
	} while (false)

#ifndef COUST_FULL_RELEASE
	// DOT NOT USE BEFORE THE CREATION OF VkDevice
	#define VK_DEBUG_NAME(name, type, handle)										\
				VkDebugUtilsObjectNameInfoEXT info									\
				{																	\
					.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,	\
					.objectType = type,												\
					.objectHandle = (uint64_t) handle,								\
					.pObjectName = name,											\
				};																	\
				vkSetDebugUtilsObjectNameEXT(::Coust::VK::g_Device, &info)
#else
	#define VK_DEBUG_NAME(name, type, handle)
#endif

namespace Coust
{
	namespace VK
	{
		extern VkDevice g_Device;
		extern VkInstance g_Instance;
		extern VkSurfaceKHR g_Surface;
		extern VmaAllocator g_VmaAlloc;
		extern VkQueue g_GraphicsQueue;
		extern VkPhysicalDevice g_PhysicalDevice;
		extern VkPhysicalDeviceProperties* g_pPhysicalDevProps;
		extern uint32_t g_GraphicsQueueFamilyIndex;
		extern uint32_t g_PresentQueueFamilyIndex;
		extern const Swapchain* g_Swapchain;
		extern bool g_AllVulkanGlobalVarSet;

		namespace Utils
		{
#ifndef COUST_FULL_RELEASE
			inline VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
				VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
				VkDebugUtilsMessageTypeFlagsEXT messageType,
				const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
				void* pUserData)
			{
				if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
					COUST_CORE_INFO(pCallbackData->pMessage);
				else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
					COUST_CORE_WARN(pCallbackData->pMessage);
				else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
					COUST_CORE_ERROR(pCallbackData->pMessage);

				return VK_FALSE;
			}

			inline VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback
			(
				VkDebugReportFlagsEXT      flags,
				VkDebugReportObjectTypeEXT objectType,
				uint64_t                   object,
				size_t                     location,
				int32_t                    messageCode,
				const char* pLayerPrefix,
				const char* pMessage,
				void* UserData
			)
			{
				// https://github.com/zeux/niagara/blob/master/src/device.cpp   [ignoring performance warnings]
				// This silences warnings like "For optimal performance image layout should be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL instead of GENERAL."
				// if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
					// return VK_FALSE;

				if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
					COUST_CORE_TRACE("{0}: {1}", pLayerPrefix, pMessage);
				else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
					COUST_CORE_INFO("{0}: {1}", pLayerPrefix, pMessage);
				else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
					COUST_CORE_WARN("{0}: {1}", pLayerPrefix, pMessage);
				else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
					COUST_CORE_WARN("{0}: {1}", pLayerPrefix, pMessage);
				else if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
					COUST_CORE_ERROR("{0}: {1}", pLayerPrefix, pMessage);

				return VK_FALSE;
			}

			inline VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo()
			{
				VkDebugUtilsMessengerCreateInfoEXT info
				{
					.sType              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
					.messageSeverity    =	VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
											VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
					.messageType        =	VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
					.pfnUserCallback    = DebugCallback,
				};
				return info;
			}

			inline VkDebugReportCallbackCreateInfoEXT DebugReportCallbackCreateInfo()
			{
				VkDebugReportCallbackCreateInfoEXT info
				{
					.sType =			VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
					.flags =			VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
										VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
					.pfnCallback =		DebugReportCallback,
				};
				return info;
			}
#endif
			inline VkImageCreateInfo GPUBasic2DImageInfo(VkFormat format, VkImageUsageFlags usage, uint32_t width, uint32_t height)
			{
				VkExtent3D extent
				{
					.width = width,
					.height = height,
					.depth = 1,
				};
				VkImageCreateInfo info
				{
					.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
					.imageType		= VK_IMAGE_TYPE_2D,
					.format			= format,
					.extent			= extent,
					.mipLevels		= 1,
					.arrayLayers	= 1,
					.samples		= VK_SAMPLE_COUNT_1_BIT,
					.tiling			= VK_IMAGE_TILING_OPTIMAL,
					.usage			= usage,
				};
				return info;
			}

			inline VkImageViewCreateInfo Basic2DImageViewInfo(VkImage image, VkFormat format, VkImageAspectFlags aspect)
			{
				VkImageViewCreateInfo info 
				{
					.sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.image              = image,
					.viewType           = VK_IMAGE_VIEW_TYPE_2D,
					.format      		= format,
					.components         = { VK_COMPONENT_SWIZZLE_IDENTITY,
        	    							VK_COMPONENT_SWIZZLE_IDENTITY,
        	    							VK_COMPONENT_SWIZZLE_IDENTITY,
        	    							VK_COMPONENT_SWIZZLE_IDENTITY },
					.subresourceRange	= {
    									  	.aspectMask 		= aspect,
    									  	.baseMipLevel 		= 0,
    									  	.levelCount 		= 1,
    									  	.baseArrayLayer 	= 0,
    									  	.layerCount 		= 1,
										  },
				};
				return info;
			}

			bool CreatePrimaryCmdBuf(VkCommandPool pool, uint32_t count, VkCommandBuffer* out_Buf);

			bool CreateSecondaryCmdBuf(VkCommandPool pool, uint32_t count, VkCommandBuffer* out_Buf);

			bool CreateSemaphore(VkSemaphore* out_Semaphore);

			bool CreateFence(bool signaled, VkFence* out_Fence);

			struct RenderPassInfo
			{
				VkFormat colorFormat = VK_FORMAT_UNDEFINED;
				bool useColor = true;
				bool useDepth = true;
				bool clearColor = false;
				bool clearDepth = false;
				bool firstPass = false;
				bool lastPass = false;
			};
			bool CreateRenderPass(const RenderPassInfo& info, VkRenderPass* out_RenderPass);

			struct FramebufferInfo
			{
				uint32_t count;
				uint32_t width, height;
				VkRenderPass renderPass;
				std::optional<VkImageView*> colorImageViews{};
				std::optional<VkImageView> depthImageView{};
			};
			bool CreateFramebuffers(const FramebufferInfo& info, VkFramebuffer* out_Framebuffers);
		}

		inline const char* VulkanReturnCodeToStr(int code)
		{
			switch (code)
			{
			case			0:		return "VK_SUCCESS";
			case			1:		return "VK_NOT_READY";
			case			2:		return "VK_TIMEOUT";
			case			3:		return "VK_EVENT_SET";
			case			4:		return "VK_EVENT_RESET";
			case			5:		return "VK_INCOMPLETE";
			case		   -1:		return "VK_ERROR_OUT_OF_HOST_MEMORY";
			case		   -2:		return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
			case		   -3:		return "VK_ERROR_INITIALIZATION_FAILED";
			case		   -4:		return "VK_ERROR_DEVICE_LOST";
			case		   -5:		return "VK_ERROR_MEMORY_MAP_FAILED";
			case		   -6:		return "VK_ERROR_LAYER_NOT_PRESENT";
			case		   -7:		return "VK_ERROR_EXTENSION_NOT_PRESENT";
			case		   -8:		return "VK_ERROR_FEATURE_NOT_PRESENT";
			case		   -9:		return "VK_ERROR_INCOMPATIBLE_DRIVER";
			case		  -10:		return "VK_ERROR_TOO_MANY_OBJECTS";
			case		  -11:		return "VK_ERROR_FORMAT_NOT_SUPPORTED";
			case		  -12:		return "VK_ERROR_FRAGMENTED_POOL";
			case		  -13:		return "VK_ERROR_UNKNOWN";
			case  -1000069000:		return "VK_ERROR_OUT_OF_POOL_MEMORY";
			case  -1000072003:		return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
			case  -1000161000:		return "VK_ERROR_FRAGMENTATION";
			case  -1000257000:		return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
			case   1000297000:		return "VK_PIPELINE_COMPILE_REQUIRED";
			case  -1000000000:		return "VK_ERROR_SURFACE_LOST_KHR";
			case  -1000000001:		return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
			case   1000001003:		return "VK_SUBOPTIMAL_KHR";
			case  -1000001004:		return "VK_ERROR_OUT_OF_DATE_KHR";
			case  -1000003001:		return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
			case  -1000011001:		return "VK_ERROR_VALIDATION_FAILED_EXT";
			case  -1000012000:		return "VK_ERROR_INVALID_SHADER_NV";
			case  -1000023000:		return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
			case  -1000023001:		return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
			case  -1000023002:		return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
			case  -1000023003:		return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
			case  -1000023004:		return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
			case  -1000023005:		return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
			case  -1000158000:		return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
			case  -1000174001:		return "VK_ERROR_NOT_PERMITTED_KHR";
			case  -1000255000:		return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
			case   1000268000:		return "VK_THREAD_IDLE_KHR";
			case   1000268001:		return "VK_THREAD_DONE_KHR";
			case   1000268002:		return "VK_OPERATION_DEFERRED_KHR";
			case   1000268003:		return "VK_OPERATION_NOT_DEFERRED_KHR";
			case  -1000338000:		return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
			default:				return "Undefined vulkan error code";
			}
		}
	}
}
