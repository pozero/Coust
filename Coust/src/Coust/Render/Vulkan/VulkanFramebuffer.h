#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"

namespace Coust::Render::VK
{
	class Framebuffer;

	class Framebuffer : Resource<VkFramebuffer, VK_OBJECT_TYPE_FRAMEBUFFER>
	{

	};
}
