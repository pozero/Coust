#include "pch.h"

#include "Coust/Renderer/Vulkan/VulkanUtils.h"

namespace Coust
{
	namespace VK
	{
		namespace Utils
		{
			bool CreatePrimaryCmdBuf(VkCommandPool pool, uint32_t count, VkCommandBuffer* out_buf)
			{
				VkCommandBufferAllocateInfo info
				{
					.sType                  = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
					.commandPool            = pool,
					.level                  = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
					.commandBufferCount     = count,
				};
				return VK_SUCCESS == vkAllocateCommandBuffers(g_Device, &info, out_buf);
			}

			bool CreateSecondaryCmdBuf(VkCommandPool pool, uint32_t count, VkCommandBuffer* out_buf)
			{
				VkCommandBufferAllocateInfo info
				{
					.sType                  = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
					.commandPool            = pool,
					.level                  = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
					.commandBufferCount     = count,
				};
				return VK_SUCCESS == vkAllocateCommandBuffers(g_Device, &info, out_buf);
			}

			bool CreateSemaphore(VkSemaphore* out_Semaphore)
			{
				VkSemaphoreCreateInfo info
				{
					.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
				};
				return VK_SUCCESS == vkCreateSemaphore(g_Device, &info, nullptr, out_Semaphore);
			}

			bool CreateFence(bool signaled, VkFence* out_Fence)
			{
				VkFenceCreateInfo info
				{
					.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
					.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : (uint32_t) 0,
				};
				return VK_SUCCESS == vkCreateFence(g_Device, &info, nullptr, out_Fence);
			}

			// TODO: Need to be modified to support multisampling
			bool CreateRenderPass(const RenderPassInfo& info, VkRenderPass* out_RenderPass)
			{
				if (!info.useColor && !info.useDepth)
					return false;

				VkAttachmentDescription colorAttach
				{
					.flags = 0,
					.format = info.colorFormat,
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.loadOp = info.clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
					.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = info.firstPass ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.finalLayout = info.lastPass ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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
					.loadOp = info.clearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
					.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = info.clearDepth ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
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
					.colorAttachmentCount = 1,
					.pColorAttachments = info.useColor ? &colorRef : nullptr,
					.pResolveAttachments = nullptr,
					.pDepthStencilAttachment = info.useDepth ? &depthRef : nullptr,
					.preserveAttachmentCount = 0,
					.pPreserveAttachments = nullptr,
				};

				VkAttachmentDescription attachs[] = { colorAttach, depthAttach };
				VkRenderPassCreateInfo renderPassInfo
				{
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.attachmentCount = uint32_t(info.useColor) + uint32_t(info.useDepth),
					.pAttachments = info.useColor ? attachs : attachs + 1,
					.subpassCount = 1,
					.pSubpasses = &subpassInfo,
					.dependencyCount = 1,
					.pDependencies = &dependency,
				};
				return VK_SUCCESS == vkCreateRenderPass(g_Device, &renderPassInfo, nullptr, out_RenderPass);
			}

			bool CreateFramebuffers(const FramebufferInfo& info, VkFramebuffer* out_Framebuffers)
			{
				VkImageView views[2]{};
				uint32_t viewCount = 0;
				
				if (info.colorImageViews.has_value())
					views[viewCount++] = info.colorImageViews.value()[0];
				if (info.depthImageView.has_value())
					views[viewCount++] = info.depthImageView.value();
				if (viewCount == 0)
					return false;

				VkFramebufferCreateInfo framebufferInfo
				{
					.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.renderPass = info.renderPass,
					.attachmentCount = viewCount,
					.pAttachments = views,
					.width = info.width,
					.height = info.height,
					.layers = 1,
				};

				bool result = true;
				for (uint32_t i = 0; i < info.count; ++i)
				{
					result = VK_SUCCESS == vkCreateFramebuffer(g_Device, &framebufferInfo, nullptr, out_Framebuffers + i);
					if (viewCount > 1)
						views[0] = info.colorImageViews.value()[i + 1];
				}
				return result;
			}
		}
	}
}
