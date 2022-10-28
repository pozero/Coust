#include "pch.h"

#include "Application.h"

#include "Event/ApplicationEvent.h"
#include "ImGui/ImGuiLayer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Coust
{
    Application* Application::s_Instance = nullptr;

    Application::Application()
    {
        COUST_CORE_ASSERT(!s_Instance, "Coust::Application Instance Already Exists");
        s_Instance = this;

        EventBus::Subscribe([this](Event& e) 
        {
            this->OnEvent(e);
        });

        m_Window = std::make_unique<Window>();

        m_ImGuiLayer = new ImGuiLayer();
        PushOverLayer(m_ImGuiLayer);

        m_Timer.Reset();
    }

    Application::~Application()
    {
    }

    void Application::Run()
    {
        while (m_IsRunning)
        {
            TimeStep ts = m_Timer.GetTimeElapsed();
            m_Timer.Reset();

            glClearColor(0.5f, 0.5f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            for (auto layer : m_LayerStack)
            {
                layer->OnUpdate(ts);
            }

            m_ImGuiLayer->Begin();
            for (auto layer : m_LayerStack)
            {
                layer->OnUIRender();
            }
            m_ImGuiLayer->End();

            m_Window->OnUpdate();
        }
    }

    void Application::OnEvent(Event& e)
    {
        // Handle Window Close
        EventBus::Dispatch<WindowClosedEvent>(e, 
            [this](WindowClosedEvent&) 
            {
                return this->Close();
            }
        );

        for (auto iter = m_LayerStack.end(); iter != m_LayerStack.begin();)
        {
            --iter;
            (*iter)->OnEvent(e);
            if (e.Handled)
                break;
        }
    }

}