#pragma once

#include "Coust/Renderer/Vulkan/VulkanUtils.h"

namespace Coust
{
	namespace VK
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
			void Cleanup();

			bool CreateRenderPass(const Param& param, VkRenderPass* out_RenderPass);

		private:
			std::vector<VkRenderPass> m_CreatedRenderPasses{};
		};

	}
}