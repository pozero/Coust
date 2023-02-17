#pragma once

#include "Coust/Core/LayerStack.h"
#include "Coust/Utils/Timer.h"

namespace Coust
{
    class Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();

        void OnEvent(Event& e);

        void PushLayer(Layer* layer)
        {
            m_LayerStack.PushLayer(layer);
        }

        void PopLayer(Layer* layer)
        {
            m_LayerStack.PopLayer(layer);
        }

        void PushOverLayer(Layer* layer)
        {
            m_LayerStack.PushOverLayer(layer);
        }

        void PopOverLayer(Layer* layer)
        {
            m_LayerStack.PopOverLayer(layer);
        }

        bool Close() 
        { 
            m_IsRunning = false; 
            return true;
        }

    private:
        LayerStack m_LayerStack;

    private:
        bool m_IsRunning = true;
        Timer m_Timer{};
    };

    Application* CreateApplication();
}
