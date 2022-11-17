#include "pch.h"

#include "Renderer.h"
#include "Vulkan/VulkanBackend.h"

namespace Coust
{
	void Renderer::Initialize()
	{
		m_Backend = new VK::Backend();
		m_Backend->Initialize();

		COUST_CORE_INFO("Renderer initialized");
	}

	void Renderer::Shutdown()
	{
		m_Backend->Shutdown();
		delete m_Backend;
	}

	void Renderer::Update()
	{

	}

	void Renderer::ImGuiBegin()
	{

	}

	void Renderer::ImGuiEnd()
	{

	}
}