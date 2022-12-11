#include "Coust/Renderer/Vulkan/VulkanUtils.h"
#include "pch.h"

#include "Coust/Renderer/Vulkan/VulkanFramebuffer.h"
#include "vulkan/vulkan_core.h"

namespace Coust
{
    namespace VK 
    {
        void FramebufferManager::Cleanup()
        {
			for (const auto& fb : m_FramebuffersAttachedToSwapchain)
			{
				for (uint32_t i = 0; i < g_Swapchain->m_CurrentSwapchainImageCount; ++i)
				{
					vkDestroyFramebuffer(g_Device, fb.framebuffer[i], nullptr);
				}
			}
        }

        bool FramebufferManager::CreateFramebuffersAttachedToSwapchain(VkRenderPass renderPass, bool useDepth, VkFramebuffer* out_Framebuffers)
        {
			Param p
			{
				.count				= g_Swapchain->m_CurrentSwapchainImageCount,
				.width				= g_Swapchain->m_Extent.width,
				.height				= g_Swapchain->m_Extent.height,
				.renderPass			= renderPass,
				.colorImageView		= g_Swapchain->GetColorImageView(),
				.depthImageView		= useDepth ? g_Swapchain->GetDepthImageView() : nullptr,
				.resolveImageViews 	= g_Swapchain->GetResolveImageViews().data(),
			};

			if (CreateFramebuffers(p, out_Framebuffers))
			{
				m_FramebuffersAttachedToSwapchain.push_back({ out_Framebuffers, renderPass, useDepth });
				return true;
			}
			else
				return false;
        }
        
        bool FramebufferManager::RecreateFramebuffersAttachedToSwapchain()
        {
			bool result = true;
			Param p
			{
				.count				= g_Swapchain->m_CurrentSwapchainImageCount,
				.width				= g_Swapchain->m_Extent.width,
				.height				= g_Swapchain->m_Extent.height,
				.renderPass			= VK_NULL_HANDLE,
				.colorImageView 	= g_Swapchain->GetColorImageView(),
				.resolveImageViews	= g_Swapchain->GetResolveImageViews().data(),
			};
			for (const auto& fb : m_FramebuffersAttachedToSwapchain)
			{
				for (uint32_t i = 0; i < g_Swapchain->m_OldSwapchainImageCount; ++i)
				{
					vkDestroyFramebuffer(g_Device, fb.framebuffer[i], nullptr);
				}

				p.renderPass = fb.renderPass;
				p.depthImageView = fb.useDepth ? g_Swapchain->GetDepthImageView() : nullptr;
				result = CreateFramebuffers(p, fb.framebuffer);
			}

			return result;
        }

        bool FramebufferManager::CreateFramebuffers(const FramebufferManager::Param& params, VkFramebuffer* out_Framebuffers)
        {
			VkImageView views[3]{};
			uint32_t viewCount = 0;
			
			if (params.colorImageView)
				views[viewCount++] = params.colorImageView;
			if (params.depthImageView)
				views[viewCount++] = params.depthImageView;
			if (params.resolveImageViews)
				views[viewCount++] = params.resolveImageViews[0];
			if (viewCount == 0)
				return false;

			VkFramebufferCreateInfo framebufferparams
			{
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.renderPass = params.renderPass,
				.attachmentCount = viewCount,
				.pAttachments = views,
				.width = params.width,
				.height = params.height,
				.layers = 1,
			};

			bool result = true;
			for (uint32_t i = 0; i < params.count; ++i)
			{
				if (params.colorImageView)
					views[viewCount-1] = params.resolveImageViews[i];
				result = VK_SUCCESS == vkCreateFramebuffer(g_Device, &framebufferparams, nullptr, out_Framebuffers + i);
			}
			return result;
        }
    }
}
