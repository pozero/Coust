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
				.colorImageViews	= g_Swapchain->GetColorImageViews().data(),
				.depthImageView		= useDepth ? g_Swapchain->GetDepthImageView() : nullptr,
			};

			if (CreateFramebuffers(p, out_Framebuffers))
			{
				m_FramebuffersAttachedToSwapchain.emplace_back(out_Framebuffers, renderPass, useDepth);
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
				.colorImageViews	= g_Swapchain->GetColorImageViews().data(),
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
			VkImageView views[2]{};
			uint32_t viewCount = 0;
			
			if (params.colorImageViews)
				views[viewCount++] = params.colorImageViews[0];
			if (params.depthImageView)
				views[viewCount++] = params.depthImageView;
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
				if (params.colorImageViews)
					views[0] = params.colorImageViews[i];
				result = VK_SUCCESS == vkCreateFramebuffer(g_Device, &framebufferparams, nullptr, out_Framebuffers + i);
			}
			return result;
        }
    }
}
