#include "pch.h"

#include "Application.h"
#include "Window.h"

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
    }

    Application::~Application()
    {
    }

    void Application::Run()
    {
        while (m_IsRunning)
        {
            glClearColor(0.5f, 0.3f, 0.7f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            float deltaTime = 1.0f / 60.0f;
            for (auto layer : m_LayerStack)
            {
                layer->OnUpdate(deltaTime);
            }

            m_Window->OnUpdate();
        }
    }

    void Application::OnEvent(Event& e)
    {
        if (e.IsInCategory("Application") || e.IsInCategory("Dull"))
            COUST_CORE_INFO(e);

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

    void Application::PushLayer(Layer* layer)
    {
        m_LayerStack.PushLayer(layer);
    }

    void Application::PopLayer(Layer* layer)
    {
        m_LayerStack.PopLayer(layer);
    }
}