#pragma once

#include "Coust/Render/Driver.h"
#include "Coust/Render/Vulkan/VulkanContext.h"
#include "Coust/Render/Vulkan/StagePool.h"
#include "Coust/Render/Vulkan/VulkanSwapchain.h"

namespace Coust::Render::VK
{
	class CommandBufferList;

    class Driver : public Coust::Render::Driver
    {
	public:
		Driver();
		virtual ~Driver();
		
		virtual void InitializationTest() override;

		virtual void LoopTest() override;

	public:
		const Context& GetContext() const { return m_Context; }
		
	private:
		bool CreateInstance();

		bool CreateDebugMessengerAndReportCallback();

		bool CreateSurface();

		bool SelectPhysicalDeviceAndCreateDevice();

    private:
        Context m_Context{};

		StagePool m_StagePool;

		Swapchain m_Swapchain;
    };
}