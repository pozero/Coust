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

        class Window* GetWindow() { return m_Window.get(); }

        static Application* GetInstance() 
        { 
            COUST_CORE_ASSERT(s_Instance, "Coust::Application Instance Not Instantiated Yet");
            return s_Instance; 
        }

    private:
        static Application* s_Instance;

    private:
        LayerStack m_LayerStack;
        std::unique_ptr<class Window> m_Window;

    private:
        bool m_IsRunning = true;
    };

    Application* CreateApplication();
}