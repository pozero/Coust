#include "pch.h"

#include "Coust/Render/Vulkan/VulkanUtils.h"

#include "Coust/Render/Vulkan/VulkanFramebuffer.h"
#include "vulkan/vulkan_core.h"

namespace Coust::Render::VK
{
	void FramebufferManager::Cleanup(const Context &ctx, const Swapchain &swapchain)
	{
		for (const auto& fb : m_FramebuffersAttachedToSwapchain)
		{
			for (uint32_t i = 0; i < swapchain.m_CurrentSwapchainImageCount; ++i)
			{
				vkDestroyFramebuffer(ctx.m_Device, fb.framebuffer[i], nullptr);
			}
		}
	}

	bool FramebufferManager::CreateFramebuffersAttachedToSwapchain(const Context &ctx, const Swapchain &swapchain, VkRenderPass renderPass, bool useDepth, VkFramebuffer* out_Framebuffers)
	{
		Param p
		{
			.count				= swapchain.m_CurrentSwapchainImageCount,
			.width				= swapchain.m_Extent.width,
			.height				= swapchain.m_Extent.height,
			.renderPass			= renderPass,
			.colorImageView		= swapchain.GetColorImageView(),
			.depthImageView		= useDepth ? swapchain.GetDepthImageView() : nullptr,
			.resolveImageViews 	= swapchain.GetResolveImageViews().data(),
		};

		if (CreateFramebuffers(ctx, p, out_Framebuffers))
		{
			m_FramebuffersAttachedToSwapchain.push_back({ out_Framebuffers, renderPass, useDepth });
			return true;
		}
		else
			return false;
	}
	
	bool FramebufferManager::RecreateFramebuffersAttachedToSwapchain(const Context &ctx, const Swapchain &swapchain)
	{
		bool result = true;
		Param p
		{
			.count				= swapchain.m_CurrentSwapchainImageCount,
			.width				= swapchain.m_Extent.width,
			.height				= swapchain.m_Extent.height,
			.renderPass			= VK_NULL_HANDLE,
			.colorImageView 	= swapchain.GetColorImageView(),
			.resolveImageViews	= swapchain.GetResolveImageViews().data(),
		};
		for (const auto& fb : m_FramebuffersAttachedToSwapchain)
		{
			for (uint32_t i = 0; i < swapchain.m_OldSwapchainImageCount; ++i)
			{
				vkDestroyFramebuffer(ctx.m_Device, fb.framebuffer[i], nullptr);
			}

			p.renderPass = fb.renderPass;
			p.depthImageView = fb.useDepth ? swapchain.GetDepthImageView() : nullptr;
			result = CreateFramebuffers(ctx, p, fb.framebuffer);
		}

		return result;
	}

	bool FramebufferManager::CreateFramebuffers(const Context &ctx, const FramebufferManager::Param& params, VkFramebuffer* out_Framebuffers)
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
			result = VK_SUCCESS == vkCreateFramebuffer(ctx.m_Device, &framebufferparams, nullptr, out_Framebuffers + i);
		}
		return result;
	}
}
