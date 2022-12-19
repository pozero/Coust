#include "Coust/Core/Logger.h"
#include "Coust/Renderer/Vulkan/VulkanUtils.h"
#include "pch.h"

#include "Coust/Renderer/Vulkan/VulkanRenderPass.h"
#include "vulkan/vulkan_core.h"
#include <stdint.h>

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

		bool RenderPassManager::CreateRenderPass(const Param& param, VkRenderPass* out_RenderPass)
		{
			if (!param.useColor && !param.useDepth)
			{
				COUST_CORE_ERROR("Can't create an empty vulkan renderpass");
				return false;
			}
			else if (!param.useColor && param.useDepth)
			{
				COUST_CORE_ERROR("Depth only renderpass not support yet");
				return false;
			}

			VkAttachmentDescription colorAttach
			{
				.flags = 0,
				.format = param.colorFormat,
				.samples = g_MSAASampleCount,
				.loadOp = param.clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = param.firstPass ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			};
			VkAttachmentReference colorRef
			{
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			};

			VkAttachmentDescription depthAttach
			{
				.flags = 0,
				.format = g_Swapchain->m_DepthFormat,
				.samples = g_MSAASampleCount,
				.loadOp = param.clearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = param.clearDepth ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			};
			VkAttachmentReference depthRef
			{
				.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			};

			VkAttachmentDescription resolveAttach 
			{
				.flags = 0,
				.format = param.colorFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = param.firstPass ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.finalLayout = param.lastPass ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			};
			VkAttachmentReference resolveRef
			{
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

			VkAttachmentDescription attachs[3];
			if (param.useColor && param.useDepth)
			{
				attachs[0] = colorAttach;
				attachs[1] = depthAttach;
				attachs[2] = resolveAttach;

				colorRef.attachment = 0;
				depthRef.attachment = 1;
				resolveRef.attachment = 2;
			}
			else if (param.useColor)
			{
				attachs[0] = colorAttach;
				attachs[1] = resolveAttach;

				colorRef.attachment = 0;
				resolveRef.attachment = 1;
			}

			VkSubpassDescription subpassInfo
			{
				.flags = 0,
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.inputAttachmentCount = 0,
				.pInputAttachments = nullptr,
				.colorAttachmentCount = param.useColor ? 1u : 0u,
				.pColorAttachments = param.useColor ? &colorRef : nullptr,
				.pResolveAttachments = param.lastPass ? &resolveRef : nullptr,
				.pDepthStencilAttachment = param.useDepth ? &depthRef : nullptr,
				.preserveAttachmentCount = 0,
				.pPreserveAttachments = nullptr,
			};

			VkRenderPassCreateInfo renderPassInfo
			{
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.attachmentCount = uint32_t(param.useColor) + uint32_t(param.useDepth) + uint32_t(param.lastPass),
				.pAttachments = attachs,
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