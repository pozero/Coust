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

        void PushLayer(Layer* layer);
        void PopLayer(Layer* layer);

        void Close() { m_IsRunning = false; }

    private:
        LayerStack m_LayerStack;

    private:
        bool m_IsRunning = true;
    };

    Application* CreateApplication();
}