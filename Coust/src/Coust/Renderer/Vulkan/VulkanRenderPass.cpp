#include "pch.h"

#include "Coust/Renderer/Vulkan/VulkanRenderPass.h"

namespace Coust
{
	namespace VK
	{
		void RenderPassManager::Cleanup()
		{
			for (VkRenderPass p : m_CreatedRenderPasses)
			{
				vkDestroyRenderPass(g_Device, p, nullptr);
			}
		}

		// TODO: Need to be modified to support multisampling
		bool RenderPassManager::CreateRenderPass(const Param& param, VkRenderPass* out_RenderPass)
		{
			if (!param.useColor && !param.useDepth)
				return false;

			VkAttachmentDescription colorAttach
			{
				.flags = 0,
				.format = param.colorFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = param.clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = param.firstPass ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.finalLayout = param.lastPass ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			};
			VkAttachmentReference colorRef
			{
				.attachment = 0,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			};

			VkAttachmentDescription depthAttach
			{
				.flags = 0,
				.format = g_Swapchain->m_DepthFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = param.clearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = param.clearDepth ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			};
			VkAttachmentReference depthRef
			{
				.attachment = 1,
				.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			};

			VkSubpassDependency dependency
			{
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
				.dependencyFlags = 0,
			};

			VkSubpassDescription subpassInfo
			{
				.flags = 0,
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.inputAttachmentCount = 0,
				.pInputAttachments = nullptr,
				.colorAttachmentCount = param.useColor ? 1u : 0u,
				.pColorAttachments = param.useColor ? &colorRef : nullptr,
				.pResolveAttachments = nullptr,
				.pDepthStencilAttachment = param.useDepth ? &depthRef : nullptr,
				.preserveAttachmentCount = 0,
				.pPreserveAttachments = nullptr,
			};

			VkAttachmentDescription attachs[] = { colorAttach, depthAttach };
			VkRenderPassCreateInfo renderPassInfo
			{
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.attachmentCount = uint32_t(param.useColor) + uint32_t(param.useDepth),
				.pAttachments = param.useColor ? attachs : attachs + 1,
				.subpassCount = 1,
				.pSubpasses = &subpassInfo,
				.dependencyCount = 1,
				.pDependencies = &dependency,
			};
			VK_CHECK(vkCreateRenderPass(g_Device, &renderPassInfo, nullptr, out_RenderPass));

			m_CreatedRenderPasses.push_back(*out_RenderPass);
			return true;
		}
	}
}