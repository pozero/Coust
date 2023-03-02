#pragma once

#include <functional>

#include "Coust/Render/Vulkan/VulkanUtils.h"

#include "Coust/Render/Driver.h"

#include "Coust/Render/Vulkan/VulkanContext.h"
#include "Coust/Render/Vulkan/VulkanShader.h"

namespace Coust::Render::VK
{
    class Driver : public Coust::Render::Driver
    {
	public:
		Driver();
		virtual ~Driver();
		
		virtual void InitializationTest() override;

	public:
		const Context& GetContext() const { return m_Context; }
		
	private:
		bool CreateInstance();

		bool CreateDebugMessengerAndReportCallback();

		bool CreateSurface();

		bool SelectPhysicalDeviceAndCreateDevice();

    private:
        Context m_Context{};
    };
}