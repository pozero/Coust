#include "pch.h"

#include "Coust/Render/Vulkan/VulkanUtils.h"

namespace Coust::Render::VK::Utils
{
	bool CreatePrimaryCmdBuf(const Context& ctx, VkCommandPool pool, uint32_t count, VkCommandBuffer* out_buf)
	{
		VkCommandBufferAllocateInfo info
		{
			.sType                  = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool            = pool,
			.level                  = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount     = count,
		};
		VK_CHECK(vkAllocateCommandBuffers(ctx.m_Device, &info, out_buf));

		return true;
	}

	bool CreateSecondaryCmdBuf(const Context& ctx, VkCommandPool pool, uint32_t count, VkCommandBuffer* out_buf)
	{
		VkCommandBufferAllocateInfo info
		{
			.sType                  = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool            = pool,
			.level                  = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
			.commandBufferCount     = count,
		};
		VK_CHECK(vkAllocateCommandBuffers(ctx.m_Device, &info, out_buf));

		return true;
	}

	bool CreateSemaphores(const Context& ctx, VkSemaphore* out_Semaphore)
	{
		VkSemaphoreCreateInfo info
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};
		VK_CHECK(vkCreateSemaphore(ctx.m_Device, &info, nullptr, out_Semaphore));

		return true;
	}

	bool CreateFence(const Context& ctx, bool signaled, VkFence* out_Fence)
	{
		VkFenceCreateInfo info
		{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : (uint32_t) 0,
		};
		VK_CHECK(vkCreateFence(ctx.m_Device, &info, nullptr, out_Fence));

		return true;
	}

	bool CreateBuffer(const Context& ctx, const Param_CreateBuffer& info, BufferAlloc* out_BufferAlloc)
	{
		VkBufferCreateInfo bufInfo
		{
			.sType  = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size   = info.allocSize,
			.usage  = info.bufUsage,
		};
		VmaAllocationCreateInfo allocInfo
		{
			.usage = info.memUsage,
		};
		VK_CHECK(vmaCreateBuffer(ctx.m_VmaAlloc, &bufInfo, &allocInfo, &out_BufferAlloc->buffer, &out_BufferAlloc->alloc, nullptr));

		return true;
	}

	bool CreateImage(const Context& ctx, const Param_CreateImage& param, ImageAlloc* out_ImageAlloc)
	{
		VkImageCreateInfo info
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.flags = param.flags,
			.imageType = param.type,
			.format = param.format,
			.extent = 
			{
				.width = param.width,
				.height = param.height,
				.depth = param.depth,
			},
			.mipLevels = param.mipLevels,
			.arrayLayers = param.arrayLayers,
			.samples = param.samples,
			.tiling = param.tiling,
			.usage = param.usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
			.initialLayout = param.initialLayout,
		};
		VmaAllocationCreateInfo allocInfo
		{
			.usage = param.memUsage,
		};
		VK_CHECK(vmaCreateImage(ctx.m_VmaAlloc, &info, &allocInfo, &out_ImageAlloc->image, &out_ImageAlloc->alloc, nullptr));

		return true;
	}

	bool CreateImageView(const Context& ctx, const Param_CreateImageView& param, VkImageView* out_ImageView)
	{
		VkImageViewCreateInfo info
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = param.imageAlloc.image,
			.viewType = param.type,
			.format = param.format,
			.components = 
			{
				.r = param.rSwizzle,
				.g = param.gSwizzle,
				.b = param.bSwizzle,
				.a = param.aSwizzle,
			},
			.subresourceRange = 
			{
				.aspectMask = param.aspect,
				.baseMipLevel = param.baseMipLevel,
				.levelCount = param.levelCount,
				.baseArrayLayer = param.baseArrayLayer,
				.layerCount = param.layerCount,
			},
		};
		VK_CHECK(vkCreateImageView(ctx.m_Device, &info, nullptr, out_ImageView));

		return true;
	}

	bool CreatePipelineLayout(const Context& ctx, const Param_CreatePipelineLayout& param, VkPipelineLayout* out_PipelineLayout)
	{
		VkPipelineLayoutCreateInfo info
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = param.descriptorSetCount,
			.pSetLayouts = param.pDescriptorSets,
			.pushConstantRangeCount = param.pushConstantRangeCount,
			.pPushConstantRanges = param.pPushConstantRanges,
		};
		VK_CHECK(vkCreatePipelineLayout(ctx.m_Device, &info, nullptr, out_PipelineLayout));

		return true;
	}
}
