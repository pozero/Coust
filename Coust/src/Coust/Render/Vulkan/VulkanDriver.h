#pragma once

#include "Coust/Render/Driver.h"
#include "Coust/Render/Vulkan/StagePool.h"
#include "Coust/Render/Vulkan/VulkanContext.h"
#include "Coust/Render/Vulkan/VulkanSwapchain.h"
#include "Coust/Render/Vulkan/VulkanRenderPass.h"
#include "Coust/Render/Vulkan/VulkanPipeline.h"

namespace Coust::Render::VK
{
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

		FBOCache m_FBOCache;

		GraphicsPipelineCache m_GraphicsPipeCache;
    };
}