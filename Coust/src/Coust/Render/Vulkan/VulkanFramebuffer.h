#pragma once

#include "Coust/Render/Vulkan/VulkanUtils.h"

namespace Coust::Render::VK
{
	class FramebufferManager
	{
	public: 
		struct Param
		{
			uint32_t count;
			uint32_t width, height;
			VkRenderPass renderPass;
			VkImageView colorImageView;
			VkImageView depthImageView;
			const VkImageView* resolveImageViews;
		};

	public:
		FramebufferManager() = default;
		~FramebufferManager() = default;
		
		void Cleanup(const Context &ctx, const Swapchain &swapchain);

		bool CreateFramebuffersAttachedToSwapchain(const Context &ctx, const Swapchain &swapchain, VkRenderPass renderPass, bool useDepth, VkFramebuffer* out_Framebuffers);
		
		bool RecreateFramebuffersAttachedToSwapchain(const Context &ctx, const Swapchain &swapchain);
		
	private:
		bool CreateFramebuffers(const Context &ctx, const Param& params, VkFramebuffer* out_Framebuffers);     

	private:
		struct AttachedToSwapchain
		{
			VkFramebuffer* framebuffer;
			VkRenderPass renderPass;
			bool useDepth;
		};
		std::vector<AttachedToSwapchain> m_FramebuffersAttachedToSwapchain{};

	};
}
