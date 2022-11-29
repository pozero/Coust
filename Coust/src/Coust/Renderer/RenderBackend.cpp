#include "pch.h"

#include "Coust/Renderer/RenderBackend.h"
#include "Coust/Renderer/Vulkan/VulkanBackend.h"

namespace Coust
{
	bool RenderBackend::Init()
	{
		return VK::Backend::Init();
	}

	void RenderBackend::Shut()
	{
		return VK::Backend::Shut();
	}

	bool RenderBackend::OnWindowResize()
	{
		return VK::Backend::RecreateSwapchainAndFramebuffers();
	}
}
