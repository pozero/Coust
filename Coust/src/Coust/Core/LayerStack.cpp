#include "pch.h"

#include "Coust/Core/LayerStack.h"

namespace Coust
{
	LayerStack::~LayerStack()
	{
		for (auto layer : m_Layers)
		{
			layer->OnDetach();
			delete layer;
		}
	}

	void LayerStack::PushLayer(Layer* layer)
	{
		m_Layers.emplace(m_Layers.begin() + (m_FirstOverLayerIndex++), layer);
		layer->OnAttach();
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		auto iter = std::find(m_Layers.begin(), m_Layers.end(), layer);
		if (iter != m_Layers.end())
		{
			layer->OnDetach();
			m_Layers.erase(iter);
			--m_FirstOverLayerIndex;
		}
	}

	void LayerStack::PushOverLayer(Layer* layer)
	{
		m_Layers.push_back(layer);
		layer->OnAttach();
	}

	void LayerStack::PopOverLayer(Layer* layer)
	{
		auto iter = std::find(m_Layers.begin(), m_Layers.end(), layer);
		if (iter != m_Layers.end())
		{
			layer->OnDetach();
			m_Layers.erase(iter);
		}
	}
}



