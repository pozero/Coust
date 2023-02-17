#pragma once

#include "Coust/Render/Vulkan/VulkanUtils.h"

namespace Coust::Render::VK
{
	class RenderPassManager
	{
	public:
		struct Param
		{
			VkFormat colorFormat = VK_FORMAT_UNDEFINED;
			bool useColor = true;
			bool useDepth = true;
			bool clearColor = false;
			bool clearDepth = false;
			bool firstPass = false;
			bool lastPass = false;
		};

	public:
		void Cleanup(const Context &ctx);

		bool CreateRenderPass(const Context &ctx, const Swapchain &swapchain, const Param& param, VkRenderPass* out_RenderPass);

	private:
		std::vector<VkRenderPass> m_CreatedRenderPasses{};
	};
}