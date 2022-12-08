#include "pch.h"

#include "Application.h"

#include "Coust/Event/ApplicationEvent.h"
#include "Coust/Renderer/RenderBackend.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Coust/Renderer/Vulkan/VulkanBackend.h"

namespace Coust
{
    Application* Application::s_Instance = nullptr;

    Application::Application()
    {
        if (s_Instance)
        {
            COUST_CORE_ERROR("Coust::Application Instance Already Exists");
            return;
        }

        s_Instance = this;

        EventBus::Subscribe([this](Event& e) 
        {
            this->OnEvent(e);
        });

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

            // ImGuiBegin();
            // for (auto layer : m_LayerStack)
            // {
            //     layer->OnUIRender();
            // }
            // ImGuiEnd();

            RenderBackend::Commit();

            glfwPollEvents();
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

        // Handle Window Resize (Vulkan Only)
        EventBus::Dispatch<WindowResizedEvent>(e,
            [](WindowResizedEvent&)
            {
                return RenderBackend::OnWindowResize();
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