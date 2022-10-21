#include "pch.h"

#include "Application.h"

namespace Coust
{
    Application::Application()
    {
    }

    Application::~Application()
    {
    }

    void Application::Run()
    {
        while (m_IsRunning)
        {
            float deltaTime = 1.0f / 60.0f;
            for (auto layer : m_LayerStack)
            {
                layer->OnUpdate(deltaTime);
            }
        }
    }

    void Application::PushLayer(Layer* layer)
    {
        m_LayerStack.PushLayer(layer);
    }

    void Application::PopLayer(Layer* layer)
    {
        m_LayerStack.PopLayer(layer);
    }
}