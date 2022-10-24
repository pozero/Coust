#pragma once

#include "LayerStack.h"

namespace Coust
{
    class Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();

        void OnEvent(Event& e);

        void PushLayer(Layer* layer);
        void PopLayer(Layer* layer);

        bool Close() 
        { 
            m_IsRunning = false; 
            return true;
        }

    private:
        LayerStack m_LayerStack;
        std::unique_ptr<class Window> m_Window;

    private:
        bool m_IsRunning = true;
    };

    Application* CreateApplication();
}