#pragma once

namespace Coust
{
	namespace VK
	{
		class Backend;
	}

	class Renderer
	{
	public:
		void Initialize();
		void Shutdown();

		void Update();

		void ImGuiBegin();
		void ImGuiEnd();

	public:
		Renderer()
		{
			Initialize();
		}

		~Renderer()
		{
			Shutdown();
		}

	private:
		VK::Backend* m_Backend;
	};
}