#pragma once

#include "Coust/Core/Logger.h"
#include "Coust/Utils/FileSystem.h"
#include "Coust/Core/Window.h"
#include "Coust/Render/Driver.h"

namespace Coust
{
	class GlobalContext
	{
	public:
		bool Initialize();
		void Shutdown();

	public:
		FileSystem& GetFileSystem() { return *g_FileSystem; }
		Window& GetWindow() { return *g_Window; }
		Render::Driver& GetRenderDriver() { return *g_RenderDriver; }

	public:
		static GlobalContext* s_Instance;
		static GlobalContext& Get() { return *s_Instance; }
		
	private:
		FileSystem* g_FileSystem = nullptr;
		Window* g_Window = nullptr;
		Render::Driver* g_RenderDriver = nullptr;
	};
}