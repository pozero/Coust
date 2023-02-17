#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

#include <cctype>

namespace Coust::Render::VK
{
	struct BufferAlloc
	{
		VkBuffer buffer;
		VmaAllocation alloc;
	};

	struct ImageAlloc
	{
		VkImage image;
		VmaAllocation alloc;
	};

	struct Texture
	{
		uint32_t width, height, depth;
		VkFormat format;

		ImageAlloc imageAlloc;
		VkImageView imageView;
		VkSampler sampler;
		VkImageLayout desiredLayout;
	};

   	struct Context
   	{
		VmaAllocator m_VmaAlloc = nullptr;

		VkInstance m_Instance = VK_NULL_HANDLE;

		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
		VkDebugReportCallbackEXT m_DebugReportCallback = VK_NULL_HANDLE;

		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

		VkDevice m_Device = VK_NULL_HANDLE;

		uint32_t m_PresentQueueFamilyIndex = (uint32_t) -1;
		uint32_t m_GraphicsQueueFamilyIndex = (uint32_t) - 1;

		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_PresentQueue = VK_NULL_HANDLE;

		VkPhysicalDeviceProperties m_PhysicalDevProps{};

		VkSampleCountFlagBits m_MSAASampleCount = VK_SAMPLE_COUNT_1_BIT;
   	};
}