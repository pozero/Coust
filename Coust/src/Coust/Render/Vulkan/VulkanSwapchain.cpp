#include "pch.h"

#include "Coust/Render/Vulkan/VulkanSwapchain.h"
#include "Coust/Render/Vulkan/VulkanCommand.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"

#include "Coust/Event/ApplicationEvent.h"
#include "Coust/Core/Window.h"

#include <GLFW/glfw3.h>

namespace Coust::Render::VK
{
	Swapchain::Swapchain(const Context &ctx)
		: Base(ctx, VK_NULL_HANDLE),
		  IsFirstRenderPass(true)
	{}

	void Swapchain::Prepare()
	{
		// prepare the information needed during creation
		{
			VkSurfaceFormatKHR bestSurfaceFormat{};
			uint32_t surfaceFormatCount = 0;
			std::vector<VkSurfaceFormatKHR> surfaceFormats{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(m_Ctx.PhysicalDevice, m_Ctx.Surface, &surfaceFormatCount, nullptr);
			surfaceFormats.resize(surfaceFormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(m_Ctx.PhysicalDevice, m_Ctx.Surface, &surfaceFormatCount, surfaceFormats.data());
			bestSurfaceFormat = surfaceFormats[0];
			for (const auto& surfaceFormat : surfaceFormats)
			{
				if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{
					bestSurfaceFormat = surfaceFormat;
					break;
				}
			}
			SurfaceFormat = bestSurfaceFormat;
		}

		{
			VkPresentModeKHR bestSurfacePresentMode = VK_PRESENT_MODE_FIFO_KHR;
			uint32_t surfacePresentModeCount = 0;
			std::vector<VkPresentModeKHR> surfacePresentModes{};
			vkGetPhysicalDeviceSurfacePresentModesKHR(m_Ctx.PhysicalDevice, m_Ctx.Surface, &surfacePresentModeCount, nullptr);
			surfacePresentModes.resize(surfacePresentModeCount);    // `surfacePresentModeCount` might be 0?
			vkGetPhysicalDeviceSurfacePresentModesKHR(m_Ctx.PhysicalDevice, m_Ctx.Surface, &surfacePresentModeCount, surfacePresentModes.data());
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
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Ctx.PhysicalDevice, m_Ctx.Surface, &surfaceCapabilities);
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
				vkGetPhysicalDeviceFormatProperties(m_Ctx.PhysicalDevice, candidates[i], &props);
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
	}

	bool Swapchain::Create()
	{
		// if the window is minimized, just wait here
		int width = 0, height = 0;
		glfwGetFramebufferSize(GlobalContext::Get().GetWindow().GetHandle(), &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(GlobalContext::Get().GetWindow().GetHandle(), &width, &height);
			glfwWaitEvents();
		}

		VkSurfaceCapabilitiesKHR surfaceCapabilities{};
		{
			VkExtent2D bestSurfaceExtent{};
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Ctx.PhysicalDevice, m_Ctx.Surface, &surfaceCapabilities);
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
			.surface         	= m_Ctx.Surface,
			.minImageCount   	= MinImageCount,
			.imageFormat     	= SurfaceFormat.format,
			.imageColorSpace 	= SurfaceFormat.colorSpace,
			.imageExtent     	= Extent,
			.imageArrayLayers 	= 1,
			.imageUsage      	= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | 
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.imageSharingMode	= VK_SHARING_MODE_EXCLUSIVE,
			.preTransform       = surfaceCapabilities.currentTransform,
			.compositeAlpha     = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode     	= PresentMode,
			.clipped            = VK_TRUE,
			.oldSwapchain       = m_Handle,
		};
	
		uint32_t queueFamilyIndices[] = 
		{
			m_Ctx.PresentQueueFamilyIndex,
			m_Ctx.GraphicsQueueFamilyIndex,
		};
		if (m_Ctx.PresentQueueFamilyIndex != m_Ctx.GraphicsQueueFamilyIndex) // graphics & present family is not the same
		{
			swapchainCreateInfo.imageSharingMode        = VK_SHARING_MODE_CONCURRENT;
			swapchainCreateInfo.queueFamilyIndexCount   = 2;
			swapchainCreateInfo.pQueueFamilyIndices     = queueFamilyIndices;
		}
		else
			swapchainCreateInfo.imageSharingMode    = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK(vkCreateSwapchainKHR(m_Ctx.Device, &swapchainCreateInfo, nullptr, &m_Handle));
	
		uint32_t imageCount;
		VK_CHECK(vkGetSwapchainImagesKHR(m_Ctx.Device, m_Handle, &imageCount, nullptr));
		m_Images.resize(imageCount);
		std::vector<VkImage> rawImg(imageCount, VK_NULL_HANDLE);
		VK_CHECK(vkGetSwapchainImagesKHR(m_Ctx.Device, m_Handle, &imageCount, rawImg.data()));
		for (uint32_t i = 0; i < imageCount; ++ i)
		{
			std::string name{ "Swapchain Attached" };
			name += std::to_string(i);
			Image::ConstructParam_Wrap p 
			{
            	.ctx = m_Ctx,
            	.handle = rawImg[i],
            	.width = Extent.width,
				.height = Extent.height,
            	.format = SurfaceFormat.format,
            	.samples = VK_SAMPLE_COUNT_1_BIT,
            	.dedicatedName = name.c_str(),
			};
			Image::Create(m_Images[i], p);
		}

		VkSemaphoreCreateInfo sci { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VK_CHECK(vkCreateSemaphore(m_Ctx.Device, &sci, nullptr, &ImgAvaiSignal));
		IsNextImgAcquired = false;

		Image::ConstructParam_Create p
		{
            .ctx = m_Ctx,
            .width = Extent.width,
            .height = Extent.height,
            .format = DepthFormat,
            .type = Image::Type::DepthStencilAttachment,
            .usageFlags = 0,
            .createFlags = 0,
            .mipLevels = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .dedicatedName = "Depth Stencil Attachment"
		};
		Image::Create(m_Depth, p);

		if (!Image::CheckValidation(*m_Depth))
		{
			COUST_CORE_ERROR("Can't create depth stencil attachment for swapchain");
			return false;
		}

		SetDedicatedDebugName("Swapchain");

		return true;
	}

	void Swapchain::Destroy()
	{
		// clear the state of command buffers before destroy swapchain
		m_Ctx.CmdBufCacheGraphics->Flush();
		m_Ctx.CmdBufCacheGraphics->Wait();

		m_Depth.reset();
		for (auto& i : m_Images)
		{
			i.reset();
		}

		vkDestroySwapchainKHR(m_Ctx.Device, m_Handle, nullptr);
		vkDestroySemaphore(m_Ctx.Device, ImgAvaiSignal, nullptr);
	}

	bool Swapchain::Acquire()
	{
		VkResult res = vkAcquireNextImageKHR(m_Ctx.Device, m_Handle, 
			std::numeric_limits<uint64_t>::max(), ImgAvaiSignal, VK_NULL_HANDLE, &m_CurImgIdx);
		
		if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
		{
			COUST_CORE_ERROR("`vkAcquireNextImageKHR` return error: {}", ToString(res));
			return false;
		}

		m_Ctx.CmdBufCacheGraphics->InjectDependency(ImgAvaiSignal);
		IsNextImgAcquired = true;

		if (res == VK_SUBOPTIMAL_KHR && !IsSubOptimal)
		{
			COUST_CORE_WARN("Suboptimal swapchain image");
			IsSubOptimal = true;
		}

		return true;
	}

	bool Swapchain::HasResized()
	{
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Ctx.PhysicalDevice, m_Ctx.Surface, &capabilities);
		return capabilities.currentExtent.width != Extent.width || capabilities.currentExtent.height != Extent.height;
	}

	void Swapchain::MakePresentable()
	{
		GetColorAttachment().TransitionLayout(m_Ctx.CmdBufCacheGraphics->Get(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 
			VkImageSubresourceRange
			{
    			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    			.baseMipLevel = 0,
    			.levelCount = 1,
    			.baseArrayLayer = 0,
    			.layerCount = 1,
			});
	}

	Image& Swapchain::GetColorAttachment() { return *m_Images[m_CurImgIdx]; }

	Image& Swapchain::GetDepthAttachment() { return *m_Depth; }

	uint32_t Swapchain::GetImageIndex() const { return m_CurImgIdx; }
}