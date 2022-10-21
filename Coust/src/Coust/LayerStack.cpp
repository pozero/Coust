#include "pch.h"

#include "LayerStack.h"

Coust::LayerStack::~LayerStack()
{
	for (auto layer : m_Layers)
	{
		layer->OnDetach();
		delete layer;
	}
}

void Coust::LayerStack::PushLayer(Layer* layer)
{
	m_Layers.emplace_back(layer);
	layer->OnAttach();
}

void Coust::LayerStack::PopLayer(Layer* layer)
{
	auto iter = std::find(m_Layers.begin(), m_Layers.end(), layer);
	if (iter != m_Layers.end())
	{
		layer->OnDetach();
		m_Layers.erase(iter);
	}
}
