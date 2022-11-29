#pragma once

namespace Coust
{
	// TODO: Add OpenGL Support
	class RenderBackend
	{
	public:
		[[nodiscard]] static bool Init();
		static void Shut();

		static bool OnWindowResize();
	};
}