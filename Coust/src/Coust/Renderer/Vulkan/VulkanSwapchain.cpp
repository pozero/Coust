#include "pch.h"

#include "Coust/Renderer/Vulkan/VulkanSwapchain.h"
#include "Coust/Renderer/Vulkan/VulkanUtils.h"
#include "vulkan/vulkan_core.h"

#include <GLFW/glfw3.h>

namespace Coust
{
	extern GLFWwindow* g_WindowHandle;
	namespace VK
	{

		bool Swapchain::Initialize()
		{
			{
				VkSurfaceFormatKHR bestSurfaceFormat{};
				uint32_t surfaceFormatCount = 0;
				std::vector<VkSurfaceFormatKHR> surfaceFormats{};
				vkGetPhysicalDeviceSurfaceFormatsKHR(g_PhysicalDevice, g_Surface, &surfaceFormatCount, nullptr);
				surfaceFormats.resize(surfaceFormatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(g_PhysicalDevice, g_Surface, &surfaceFormatCount, surfaceFormats.data());
				bestSurfaceFormat = surfaceFormats[0];
				for (const auto& surfaceFormat : surfaceFormats)
				{
					if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
					{
						bestSurfaceFormat = surfaceFormat;
						break;
					}
				}
				m_Format = bestSurfaceFormat;
			}
		
			{
				VkPresentModeKHR bestSurfacePresentMode = VK_PRESENT_MODE_FIFO_KHR;
   				uint32_t surfacePresentModeCount = 0;
   				std::vector<VkPresentModeKHR> surfacePresentModes{};
   				vkGetPhysicalDeviceSurfacePresentModesKHR(g_PhysicalDevice, g_Surface, &surfacePresentModeCount, nullptr);
   				surfacePresentModes.resize(surfacePresentModeCount);    // `surfacePresentModeCount` might be 0?
   				vkGetPhysicalDeviceSurfacePresentModesKHR(g_PhysicalDevice, g_Surface, &surfacePresentModeCount, surfacePresentModes.data());
				for (const auto& mode : surfacePresentModes)
				{
					if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
					{
						bestSurfacePresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
						break;
					}
				}
				m_PresentMode = bestSurfacePresentMode;
			}
		
			{
   				VkSurfaceCapabilitiesKHR surfaceCapabilities{};
   				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_PhysicalDevice, g_Surface, &surfaceCapabilities);
				uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
				if (surfaceCapabilities.maxImageCount != 0 && imageCount > surfaceCapabilities.maxImageCount)
					imageCount = surfaceCapabilities.maxImageCount;
				m_MinImageCount = imageCount;
			}

			{
				VkFormat bestDepthFormat = VK_FORMAT_UNDEFINED;
				VkFormat candidates[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
				for (uint32_t i = 0; i < ARRAYSIZE(candidates); ++i)
				{
					VkFormatProperties props{};
					vkGetPhysicalDeviceFormatProperties(g_PhysicalDevice, candidates[i], &props);
					// select optimal tiling
					if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
					{
						bestDepthFormat = candidates[i];
						// TODO: We don't need stencil buffer now
						break;
					}
				}
				if (bestDepthFormat == VK_FORMAT_UNDEFINED)
				{
					COUST_CORE_ERROR("Can't find appropriate depth image format for vulkan");
					return false;
				}
				m_DepthFormat = bestDepthFormat;
			}

			return true;
		}

		bool Swapchain::Create()
		{
   			VkSurfaceCapabilitiesKHR surfaceCapabilities{};
			{
				VkExtent2D bestSurfaceExtent{};
   				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_PhysicalDevice, g_Surface, &surfaceCapabilities);
				if (surfaceCapabilities.currentExtent.width == (uint32_t) -1)
   				{
   					int width, height;
   					glfwGetFramebufferSize(g_WindowHandle, &width, &height);
					uint32_t actualWidth  = std::clamp((uint32_t) width,  surfaceCapabilities.minImageExtent.width,  surfaceCapabilities.maxImageExtent.width);
					uint32_t actualHeight = std::clamp((uint32_t) height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
					bestSurfaceExtent = VkExtent2D{ actualWidth, actualHeight };
				}
				else
   					bestSurfaceExtent = surfaceCapabilities.currentExtent;

				m_Extent = bestSurfaceExtent;
			}

			VkSwapchainCreateInfoKHR swapchainCreateInfo
			{
				.sType           	= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
				.surface         	= g_Surface,
				.minImageCount   	= m_MinImageCount,
				.imageFormat     	= m_Format.format,
				.imageColorSpace 	= m_Format.colorSpace,
				.imageExtent     	= m_Extent,
				.imageArrayLayers 	= 1,
				// the image can be updated by command buffer
				// TODO: May need to support screenshot?
				.imageUsage      	= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,	
				.imageSharingMode	= VK_SHARING_MODE_EXCLUSIVE,
				.preTransform       = surfaceCapabilities.currentTransform,
				// TODO: We don't need alpha composition now
				.compositeAlpha     = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
				.presentMode     	= m_PresentMode,
				.clipped            = VK_TRUE,
 				.oldSwapchain       = m_Swapchain,
			};
		
			uint32_t queueFamilyIndices[] = 
			{
				g_PresentQueueFamilyIndex,
				g_GraphicsQueueFamilyIndex,
			};
			if (g_PresentQueueFamilyIndex != g_GraphicsQueueFamilyIndex) // graphics & present family is not the same
			{
				swapchainCreateInfo.imageSharingMode        = VK_SHARING_MODE_CONCURRENT;
				swapchainCreateInfo.queueFamilyIndexCount   = 2;
				swapchainCreateInfo.pQueueFamilyIndices     = queueFamilyIndices;
			}
			else
				swapchainCreateInfo.imageSharingMode    = VK_SHARING_MODE_EXCLUSIVE;
		
			{
				VK_CHECK(vkCreateSwapchainKHR(g_Device, &swapchainCreateInfo, nullptr, &m_Swapchain));
			}
		
			{
				vkGetSwapchainImagesKHR(g_Device, m_Swapchain, &m_CurrentSwapchainImageCount, nullptr);
				m_Images.resize(m_CurrentSwapchainImageCount);
				vkGetSwapchainImagesKHR(g_Device, m_Swapchain, &m_CurrentSwapchainImageCount, m_Images.data());
			}
		
			{
				m_ImageViews.resize(m_Images.size());
				for (int i = 0; i < m_Images.size(); ++i)
				{
					VkImageViewCreateInfo imageViewInfo
					{
						.sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
						.image              = m_Images[i],
						.viewType           = VK_IMAGE_VIEW_TYPE_2D,
						.format      		= m_Format.format,
						.components         = { VK_COMPONENT_SWIZZLE_IDENTITY,
												VK_COMPONENT_SWIZZLE_IDENTITY,
												VK_COMPONENT_SWIZZLE_IDENTITY,
												VK_COMPONENT_SWIZZLE_IDENTITY },
						.subresourceRange	= {
											  	.aspectMask 		= VK_IMAGE_ASPECT_COLOR_BIT,
											  	.baseMipLevel 		= 0,
											  	.levelCount 		= 1,
											  	.baseArrayLayer 	= 0,
											  	.layerCount 		= 1,
											  },
					};
					vkCreateImageView(g_Device, &imageViewInfo, nullptr, &m_ImageViews[i]);
				}
			}

			{	// Color Image
				VkExtent3D extent 
				{
					.width = m_Extent.width,
					.height = m_Extent.height,
					.depth = 1,
				};
				VkImageCreateInfo imageInfo
				{
					.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
					.imageType		= VK_IMAGE_TYPE_2D,
					.format			= m_Format.format,
					.extent			= extent,
					.mipLevels		= 1,
					.arrayLayers	= 1,
					.samples		= g_MSAASampleCount,
					.tiling			= VK_IMAGE_TILING_OPTIMAL,
					.usage			= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				};
				VmaAllocationCreateInfo allocInfo 
				{
					.usage = VMA_MEMORY_USAGE_GPU_ONLY,
					.requiredFlags = (VkMemoryPropertyFlags) VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				};
				VK_CHECK(vmaCreateImage(g_VmaAlloc, &imageInfo, &allocInfo, &m_ColorImageAlloc.image, &m_ColorImageAlloc.alloc, nullptr));
				VkImageViewCreateInfo imageViewInfo
				{
					.sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.image              = m_ColorImageAlloc.image,
					.viewType           = VK_IMAGE_VIEW_TYPE_2D,
					.format      		= m_Format.format,
					.components         = { VK_COMPONENT_SWIZZLE_IDENTITY,
											VK_COMPONENT_SWIZZLE_IDENTITY,
											VK_COMPONENT_SWIZZLE_IDENTITY,
											VK_COMPONENT_SWIZZLE_IDENTITY },
					.subresourceRange	= {
										  	.aspectMask 		= VK_IMAGE_ASPECT_COLOR_BIT,
										  	.baseMipLevel 		= 0,
										  	.levelCount 		= 1,
										  	.baseArrayLayer 	= 0,
										  	.layerCount 		= 1,
										  },
				};
				VK_CHECK(vkCreateImageView(g_Device, &imageViewInfo, nullptr, &m_ColorImageView));
			}
		
			{	// Depth Image (Buffer)
				VkExtent3D extent 
				{
					.width = m_Extent.width,
					.height = m_Extent.height,
					.depth = 1,
				};
				VkImageCreateInfo imageInfo
				{
					.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
					.imageType		= VK_IMAGE_TYPE_2D,
					.format			= m_DepthFormat,
					.extent			= extent,
					.mipLevels		= 1,
					.arrayLayers	= 1,
					.samples		= g_MSAASampleCount,
					.tiling			= VK_IMAGE_TILING_OPTIMAL,
					.usage			= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				};
				VmaAllocationCreateInfo allocInfo 
				{
					.usage = VMA_MEMORY_USAGE_GPU_ONLY,
					.requiredFlags = (VkMemoryPropertyFlags) VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				};
				VK_CHECK(vmaCreateImage(g_VmaAlloc, &imageInfo, &allocInfo, &m_DepthImageAlloc.image, &m_DepthImageAlloc.alloc, nullptr));
				VkImageViewCreateInfo imageViewInfo
				{
					.sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.image              = m_DepthImageAlloc.image,
					.viewType           = VK_IMAGE_VIEW_TYPE_2D,
					.format      		= m_DepthFormat,
					.components         = { VK_COMPONENT_SWIZZLE_IDENTITY,
											VK_COMPONENT_SWIZZLE_IDENTITY,
											VK_COMPONENT_SWIZZLE_IDENTITY,
											VK_COMPONENT_SWIZZLE_IDENTITY },
					.subresourceRange	= {
										  	.aspectMask 		= VK_IMAGE_ASPECT_DEPTH_BIT,
										  	.baseMipLevel 		= 0,
										  	.levelCount 		= 1,
										  	.baseArrayLayer 	= 0,
										  	.layerCount 		= 1,
										  },
				};
				VK_CHECK(vkCreateImageView(g_Device, &imageViewInfo, nullptr, &m_DepthImageView));
			}

			return true;
		}

		void Swapchain::Cleanup()
		{
			vkDestroyImageView(g_Device, m_DepthImageView, nullptr);  
			vmaDestroyImage(g_VmaAlloc, m_DepthImageAlloc.image, m_DepthImageAlloc.alloc); 

			vkDestroyImageView(g_Device, m_ColorImageView, nullptr);
			vmaDestroyImage(g_VmaAlloc, m_ColorImageAlloc.image, m_ColorImageAlloc.alloc);

			m_DepthImageView = VK_NULL_HANDLE;
			m_ColorImageView = VK_NULL_HANDLE;
		
			for (auto& imageview : m_ImageViews)
			{
				vkDestroyImageView(g_Device, imageview, nullptr);
			}
			m_CurrentSwapchainImageCount = 0;
		
			if (!m_Recreation)
			{
				vkDestroySwapchainKHR(g_Device, m_Swapchain, nullptr);
				m_Swapchain = VK_NULL_HANDLE;
			}
		}

		bool Swapchain::Recreate()
		{
			int width = 0, height = 0;
			glfwGetFramebufferSize(g_WindowHandle, &width, &height);
			while (width == 0 || height == 0)
			{
				glfwGetFramebufferSize(g_WindowHandle, &width, &height);
				glfwWaitEvents();
			}
			vkDeviceWaitIdle(g_Device);

			m_Recreation = true;
			VkSwapchainKHR old = m_Swapchain;
			m_OldSwapchainImageCount = m_CurrentSwapchainImageCount;

			Cleanup();
			bool result = Create();

			m_Recreation = false;

			if (result)
				vkDestroySwapchainKHR(g_Device, old, nullptr);
			else
				m_Swapchain = old;

			return result;
		}
	}
}
