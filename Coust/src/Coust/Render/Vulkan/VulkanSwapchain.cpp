#include "pch.h"

#include "Coust/Event/ApplicationEvent.h"

#include "Coust/Render/Vulkan/VulkanSwapchain.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "vulkan/vulkan_core.h"

#include <GLFW/glfw3.h>

namespace Coust::Render::VK
{
	Swapchain::Swapchain(const Context &ctx)
		: Resource(ctx, VK_NULL_HANDLE)
	{
		{
			VkSurfaceFormatKHR bestSurfaceFormat{};
			uint32_t surfaceFormatCount = 0;
			std::vector<VkSurfaceFormatKHR> surfaceFormats{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.PhysicalDevice, ctx.Surface, &surfaceFormatCount, nullptr);
			surfaceFormats.resize(surfaceFormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.PhysicalDevice, ctx.Surface, &surfaceFormatCount, surfaceFormats.data());
			bestSurfaceFormat = surfaceFormats[0];
			for (const auto& surfaceFormat : surfaceFormats)
			{
				if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{
					bestSurfaceFormat = surfaceFormat;
					break;
				}
			}
			Format = bestSurfaceFormat;
		}

		{
			VkPresentModeKHR bestSurfacePresentMode = VK_PRESENT_MODE_FIFO_KHR;
			uint32_t surfacePresentModeCount = 0;
			std::vector<VkPresentModeKHR> surfacePresentModes{};
			vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.PhysicalDevice, ctx.Surface, &surfacePresentModeCount, nullptr);
			surfacePresentModes.resize(surfacePresentModeCount);    // `surfacePresentModeCount` might be 0?
			vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.PhysicalDevice, ctx.Surface, &surfacePresentModeCount, surfacePresentModes.data());
			for (const auto& mode : surfacePresentModes)
			{
				if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					bestSurfacePresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}
			}
			PresentMode = bestSurfacePresentMode;
		}
	
		{
			VkSurfaceCapabilitiesKHR surfaceCapabilities{};
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.PhysicalDevice, ctx.Surface, &surfaceCapabilities);
			uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
			if (surfaceCapabilities.maxImageCount != 0 && imageCount > surfaceCapabilities.maxImageCount)
				imageCount = surfaceCapabilities.maxImageCount;
			MinImageCount = imageCount;
		}

		{
			VkFormat bestDepthFormat = VK_FORMAT_UNDEFINED;
			VkFormat candidates[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
			for (uint32_t i = 0; i < ARRAYSIZE(candidates); ++i)
			{
				VkFormatProperties props{};
				vkGetPhysicalDeviceFormatProperties(ctx.PhysicalDevice, candidates[i], &props);
				// select optimal tiling
				if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
				{
					bestDepthFormat = candidates[i];
					// TODO: We don't need stencil buffer now
					break;
				}
			}

			COUST_CORE_ASSERT(bestDepthFormat != VK_FORMAT_UNDEFINED, "Can't find appropriate depth image format for vulkan");
			DepthFormat = bestDepthFormat;
		}
		
		m_IsValid = Create(ctx);
		if (m_IsValid)
			SetDedicatedDebugName(std::string{ "Swapchain" });
	}

	Swapchain::~Swapchain()
	{
		if (m_Handle)
			vkDestroySwapchainKHR(m_Device, m_Handle, nullptr);
	}

	bool Swapchain::Create(const Context &ctx)
	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities{};
		{
			VkExtent2D bestSurfaceExtent{};
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.PhysicalDevice, ctx.Surface, &surfaceCapabilities);
			if (surfaceCapabilities.currentExtent.width == (uint32_t) -1)
			{
				int width, height;
				glfwGetFramebufferSize(GlobalContext::Get().GetWindow().GetHandle(), &width, &height);
				uint32_t actualWidth  = std::clamp((uint32_t) width,  surfaceCapabilities.minImageExtent.width,  surfaceCapabilities.maxImageExtent.width);
				uint32_t actualHeight = std::clamp((uint32_t) height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
				bestSurfaceExtent = VkExtent2D{ actualWidth, actualHeight };
			}
			else
				bestSurfaceExtent = surfaceCapabilities.currentExtent;

			Extent = bestSurfaceExtent;
		}

		VkSwapchainCreateInfoKHR swapchainCreateInfo
		{
			.sType           	= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface         	= ctx.Surface,
			.minImageCount   	= MinImageCount,
			.imageFormat     	= Format.format,
			.imageColorSpace 	= Format.colorSpace,
			.imageExtent     	= Extent,
			.imageArrayLayers 	= 1,
			// the image can be updated by command buffer
			// TODO: May need to support screenshot?
			.imageUsage      	= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,	
			.imageSharingMode	= VK_SHARING_MODE_EXCLUSIVE,
			.preTransform       = surfaceCapabilities.currentTransform,
			// TODO: We don't need alpha composition now
			.compositeAlpha     = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode     	= PresentMode,
			.clipped            = VK_TRUE,
			.oldSwapchain       = m_Handle,
		};
	
		uint32_t queueFamilyIndices[] = 
		{
			ctx.PresentQueueFamilyIndex,
			ctx.GraphicsQueueFamilyIndex,
		};
		if (ctx.PresentQueueFamilyIndex != ctx.GraphicsQueueFamilyIndex) // graphics & present family is not the same
		{
			swapchainCreateInfo.imageSharingMode        = VK_SHARING_MODE_CONCURRENT;
			swapchainCreateInfo.queueFamilyIndexCount   = 2;
			swapchainCreateInfo.pQueueFamilyIndices     = queueFamilyIndices;
		}
		else
			swapchainCreateInfo.imageSharingMode    = VK_SHARING_MODE_EXCLUSIVE;
	
		VK_CHECK(vkCreateSwapchainKHR(ctx.Device, &swapchainCreateInfo, nullptr, &m_Handle));
	
		VK_CHECK(vkGetSwapchainImagesKHR(ctx.Device, m_Handle, &CurrentSwapchainImageCount, nullptr));
		m_Images.resize(CurrentSwapchainImageCount);
		VK_CHECK(vkGetSwapchainImagesKHR(ctx.Device, m_Handle, &CurrentSwapchainImageCount, m_Images.data()));
	
		return true;
	}

	bool Swapchain::Recreate(const Context &ctx)
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(GlobalContext::Get().GetWindow().GetHandle(), &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(GlobalContext::Get().GetWindow().GetHandle(), &width, &height);
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(m_Device);

		VkSwapchainKHR old = m_Handle;
		if (Create(ctx))
		{
			vkDestroySwapchainKHR(m_Device, old, nullptr);
			return true;
		}
		else
		{
			m_Handle = old;
			m_IsValid = false;
			return false;
		}
	}
	
	VkResult Swapchain::AcquireNextImage(uint64_t timeOut, VkSemaphore semaphoreToSignal, VkFence fenceToSignal, uint32_t* out_ImageIndex)
	{
		return vkAcquireNextImageKHR(m_Device, m_Handle, timeOut, semaphoreToSignal, fenceToSignal, out_ImageIndex);
	}
}
