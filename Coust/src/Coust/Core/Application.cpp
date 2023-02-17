#include "pch.h"

#include "Application.h"

#include "Coust/Event/ApplicationEvent.h"
#include "Coust/Core/Window.h"
#include "Coust/Utils/FileSystem.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Coust
{
    Application::Application()
    {
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

            glfwPollEvents();

            GlobalContext::Get().GetRenderDriver().FlushCommand();
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
                return GlobalContext::Get().GetRenderDriver().RecreateSwapchainAndFramebuffers();
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
