#include "pch.h"

#include "Application.h"

#include "Event/ApplicationEvent.h"

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
        WindowResizedEvent e{200, 100};
        COUST_CORE_TRACE(e);
        std::string appCategory{ "Application" };
        COUST_CORE_TRACE("In Category {0}: {1}", appCategory, e.IsInCategory(appCategory));

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