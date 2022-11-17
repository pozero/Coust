#pragma once

#include "Coust/Window.h"
#include "Coust/LayerStack.h"
#include "Coust/Timer.h"
#include "Coust/Renderer/Renderer.h"

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

        class Window& GetWindow() { return *m_Window.get(); }

        static Application& GetInstance() 
        { 
            COUST_CORE_ASSERT(s_Instance, "Coust::Application Instance Not Instantiated Yet");
            return *s_Instance; 
        }

    private:
        static Application* s_Instance;

    private:
        LayerStack m_LayerStack;
        std::unique_ptr<class Window> m_Window;
        std::unique_ptr<Renderer> m_Renderer;

    private:
        bool m_IsRunning = true;
        Timer m_Timer{};
    };

    Application* CreateApplication();
}