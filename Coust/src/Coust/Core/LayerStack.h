#pragma once

#include "Coust/Core/Layer.h"

namespace Coust
{
	class LayerStack
	{
	public:
		LayerStack() = default;

		~LayerStack();

		void PushLayer(Layer* layer);
		void PopLayer(Layer* layer);

		void PushOverLayer(Layer* layer);
		void PopOverLayer(Layer* layer);

		std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
		std::vector<Layer*>::iterator end() { return m_Layers.end(); }

		std::vector<Layer*>::const_iterator begin() const { return m_Layers.begin(); }
		std::vector<Layer*>::const_iterator end() const { return m_Layers.end(); }

	private:
		std::vector<Layer*> m_Layers;
		int m_FirstOverLayerIndex = 0;
	};
}
