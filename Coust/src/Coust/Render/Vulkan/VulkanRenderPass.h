#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"

namespace Coust::Render::VK
{
	class RenderPass;

	class RenderPass : public Resource<VkRenderPass, VK_OBJECT_TYPE_RENDER_PASS>
	{

	};
}