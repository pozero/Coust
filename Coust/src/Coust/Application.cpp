#include "pch.h"

#include "Application.h"

#include "Event/ApplicationEvent.h"

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

        m_Renderer = std::make_unique<Renderer>();

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

            for (auto layer : m_LayerStack)
            {
                layer->OnUpdate(ts);
            }

            m_Renderer->ImGuiBegin();
            for (auto layer : m_LayerStack)
            {
                layer->OnUIRender();
            }
            m_Renderer->ImGuiEnd();

            glfwPollEvents();

            m_Renderer->Update();
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