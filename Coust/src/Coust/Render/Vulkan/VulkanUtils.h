#pragma once

#include "Coust/Core/Logger.h"

#include <volk.h>

#include <vector>

#define VK_CHECK(func)																													\
	do																																	\
	{																																	\
		VkResult err = func;																											\
		if (err != VK_SUCCESS)																											\
		{																																\
			COUST_CORE_ERROR("File {0}, Line{1}, Vulkan Func {2} return {3}", __FILE__, __LINE__, #func, ToString(err));	\
			return false;																												\
		}																																\
	} while (false)

#define VK_REPORT(func, status)																											\
	do																																	\
	{																																	\
		VkResult err = func;																											\
		if (err != VK_SUCCESS)																											\
		{																																\
			status = false;																												\
			COUST_CORE_ERROR("File {0}, Line{1}, Vulkan Func {2} return {3}", __FILE__, __LINE__, #func, ToString(err));	\
		}																																\
		else																															\
			status = true;																												\
	} while (false)

namespace Coust::Render::VK
{
	constexpr uint32_t VULKAN_API_VERSION = VK_API_VERSION_1_2;

	inline VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		/* Redundant validation error message */

		// if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		// 	COUST_CORE_INFO(pCallbackData->pMessage);
		// else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		// 	COUST_CORE_WARN(pCallbackData->pMessage);
		// else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		// 	COUST_CORE_ERROR(pCallbackData->pMessage);

		/* Redundant validation error message */

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
		return VkDebugUtilsMessengerCreateInfoEXT
		{
			.sType              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity    =	VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType        =	VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback    = DebugCallback,
		};
	}

	inline VkDebugReportCallbackCreateInfoEXT DebugReportCallbackCreateInfo()
	{
		return VkDebugReportCallbackCreateInfoEXT
		{
			.sType =			VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
			.flags =			VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
								VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
			.pfnCallback =		DebugReportCallback,
		};
	}

		// struct Param_CreateBuffer
		// {
		//     VkDeviceSize allocSize; 
		//     VkBufferUsageFlags bufUsage; 
		//     VmaMemoryUsage memUsage;
		// };

		// struct Param_CreateImage
		// {
		// 	VkImageCreateFlags flags = 0u;
		// 	VkImageType type = VK_IMAGE_TYPE_2D;
		// 	VkFormat format;
		// 	uint32_t width, height;
		// 	uint32_t depth = 1u;
		// 	uint32_t mipLevels = 1;
		// 	uint32_t arrayLayers = 1;
		// 	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
		// 	VkImageTiling tiling;
		// 	VkImageUsageFlags usage;
		// 	VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        //  VmaMemoryUsage memUsage;
		// };

		// struct Param_CreateImageView
		// {
		// 	const ImageAlloc& imageAlloc;
		// 	VkImageViewType type;
		// 	VkFormat format;
		// 	VkComponentSwizzle rSwizzle = VK_COMPONENT_SWIZZLE_IDENTITY;
		// 	VkComponentSwizzle gSwizzle = VK_COMPONENT_SWIZZLE_IDENTITY;
		// 	VkComponentSwizzle bSwizzle = VK_COMPONENT_SWIZZLE_IDENTITY;
		// 	VkComponentSwizzle aSwizzle = VK_COMPONENT_SWIZZLE_IDENTITY;
		// 	VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		// 	uint32_t baseMipLevel = 0, levelCount = 1;
		// 	uint32_t baseArrayLayer = 0, layerCount = 1;
		// };

		// struct Param_CreatePipelineLayout
		// {
		// 	uint32_t descriptorSetCount;
		// 	const VkDescriptorSetLayout* pDescriptorSets;
		// 	uint32_t pushConstantRangeCount = 0;
		// 	const VkPushConstantRange* pPushConstantRanges = nullptr;
		// };

	template <typename T>
    inline uint32_t ToU32(T value)
        requires std::is_arithmetic<T>::value
    {
        COUST_CORE_ASSERT(static_cast<uintmax_t>(value) <= static_cast<uintmax_t>(std::numeric_limits<uint32_t>::max()), 
						  "ToU32() failed, value is too big to be converted to uint32_t");
        return static_cast<uint32_t>(value);
    }

	const char* ToString(VkResult result);
	
	const char* ToString(VkObjectType objType);
	
    const char* ToString(VkAccessFlagBits bit);
    
    const char* ToString(VkShaderStageFlagBits bit);
	
	const char* ToString(VkCommandBufferLevel level);
	
	const char* ToString(VkDescriptorType type);
	
	const char* ToString(VkBufferUsageFlagBits bit);

	const char* ToString(VkImageUsageFlagBits bit);
	
    template <typename Flag, typename FlagBit>
    inline std::string ToString(Flag flags)
        requires std::is_arithmetic<Flag>::value && std::is_enum<FlagBit>::value
    {
        std::string res{};
        for (FlagBit bit = (FlagBit) 1u; uintmax_t(bit) > 0u; bit = FlagBit(uintmax_t(bit) << 1))
        {
            if (bit & flags)
            {
                if (!res.empty())
                    res.append(" | ");
                res.append(ToString(bit));
            }
        }
        return res;
    }
}

namespace std
{
    template <>
    struct hash<VkDescriptorBufferInfo>
	{
		std::size_t operator()(const VkDescriptorBufferInfo& key);
	};

    template <>
    struct hash<VkDescriptorImageInfo>
	{
		std::size_t operator()(const VkDescriptorImageInfo& key);
	};

    template <>
    struct hash<VkWriteDescriptorSet>
	{
		std::size_t operator()(const VkWriteDescriptorSet& key);
	};
}
