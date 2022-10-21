#pragma once

#include "pch.h"

namespace Coust
{
	class Layer
	{
	public:
		Layer(const std::string& name = "Layer") : m_Name(name) {}

		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}

		virtual void OnUpdate(float deltaTime) {}

		const std::string& GetName() { return m_Name; }
	private:
		std::string m_Name;
	};
}