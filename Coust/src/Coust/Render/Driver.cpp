#include "pch.h"

#include "Coust/Render/Driver.h"
#include "Coust/Render/Vulkan/VulkanDriver.h"

namespace Coust::Render
{
    Driver* Coust::Render::Driver::CreateDriver(API api)
	{
		Driver* driver = nullptr;

		switch (api)
		{
		case API::VULKAN:
			driver = new VK::Driver();
			if (!driver->Initialize())
			{
				driver->Shutdown();
				delete driver;
				driver = nullptr;
			}
		}

		return driver;
	}
}
