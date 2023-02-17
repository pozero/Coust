#include "pch.h"
#include "GlobalContext.h"

namespace Coust
{
	GlobalContext* GlobalContext::s_Instance = nullptr;

	bool GlobalContext::Initialize()
	{
		if (s_Instance)
		{
			COUST_CORE_ERROR("Coust::GlobalContext Initialized More Than Once");
			return false;
		}
		s_Instance = this;

		if (!Coust::Logger::Initialize())
		{
			std::cerr << "Logger Initialization Failed.\n";
			return false;
		}
	
		if (g_FileSystem = FileSystem::CreateFileSystem(); !g_FileSystem)
		{
			COUST_CORE_ERROR("File System Initialization Failed.");
			return false;
		}
	
		if (g_Window = Window::CreateCoustWindow(); !g_Window)
		{
			COUST_CORE_ERROR("Window Initialization Failed.");
			return false;
		}
	
		if (g_RenderDriver = Render::Driver::CreateDriver(Render::API::VULKAN); !g_RenderDriver)
		{
			COUST_CORE_ERROR("Render System Initialization Failed");
			return false;
		}
	
		return true;
	}
	
	void GlobalContext::Shutdown()
	{
		if (g_RenderDriver)
		{
			g_RenderDriver->Shutdown();
			delete g_RenderDriver;
		}

		if (g_Window)
		{
			g_Window->Shutdown();
			delete g_Window;
		}

		if (g_FileSystem)
		{
			g_FileSystem->Shutdown();
			delete g_FileSystem;
		}

		Logger::Shutdown();
	}
}
