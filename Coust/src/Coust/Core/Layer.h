#pragma once

#include "Coust/Event/Event.h"
#include "Coust/Utils/Timer.h"

namespace Coust
{
	class Layer
	{
	public:
		Layer(const std::string& name = "Layer") : m_Name(name) {}

		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}

		virtual void OnEvent(Event& e) {}

		virtual void OnUpdate(const TimeStep& ts) {}

		virtual void OnUIRender() {}

		const std::string& GetName() { return m_Name; }
	private:
		std::string m_Name;
	};
}
