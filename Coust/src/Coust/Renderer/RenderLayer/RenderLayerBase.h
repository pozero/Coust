#pragma once

#include "Coust/Renderer/Vulkan/VulkanUtils.h"

#include <vector>

namespace Coust
{
	class RenderLayerBase
	{
	public:

	protected:
		// void BegingRenderPass(VkCommandBuffer cmd, uint32_t swapchainImageIndex);

	// Per layer resources
	protected:
		VkRenderPass m_RenderPass;

		std::vector<VkFramebuffer> m_Framebuffers;

		// pipeline per layer class
		// static VkPipelineLayout m_PipelineLayout;
		// static VkPipeline m_Pipeline;

	// Global/Per renderer resources
	protected:
		// binding point 0
		// VkDescriptorSet* const s_RendererDescriptorSets = nullptr;
		// VkDescriptorSetLayout const s_RendererDescriptorSetLayout = VK_NULL_HANDLE;
	};
}