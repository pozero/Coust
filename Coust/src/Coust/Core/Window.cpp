#include "pch.h"

#include "Coust/Core/Window.h"
#include "Coust/Core/Application.h"
#include "Coust/Event/KeyEvent.h"
#include "Coust/Event/MouseEvent.h"
#include "Coust/Event/ApplicationEvent.h"

#include <GLFW/glfw3.h>

namespace Coust
{
    Window* Window::s_Window = nullptr;
	GLFWwindow* g_WindowHandle;
    
    bool Window::Init()
    {
        if (s_Window)
        {
            COUST_CORE_ERROR("Window instance already exists");
            return false;
        }

        s_Window = new Window();
        return s_Window->Initialize();
    }

    void Window::Shut()
    {
        if (s_Window)
        {
            s_Window->Shutdown();
            s_Window = nullptr;
        }
    }

    Window::Window(const Config& config)
    {
        m_WindowProp = config;
    }

    static void GLFWErrorCallback(int error, const char* desc)
    {
        COUST_CORE_ERROR("GLFW Error ({0}): {1}", error, desc);
    }

    bool Window::Initialize()
    {
        int glfwInitializationSuccess = glfwInit();
        if (!glfwInitializationSuccess)
        {
            COUST_CORE_ERROR("GLFW Initialization Falied");
            return false;
        }

        glfwSetErrorCallback(GLFWErrorCallback);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_WindowHandle = glfwCreateWindow(m_WindowProp.width, m_WindowProp.height, m_WindowProp.name, nullptr, nullptr);
        if (!m_WindowHandle)
        {
            COUST_CORE_ERROR("GLFW Window Creation Failed");
            return false;
        }
        g_WindowHandle = m_WindowHandle;

        /* Set Window Event Callbacks */
        glfwSetKeyCallback(m_WindowHandle, 
            [](GLFWwindow* window, int key, int scancode, int action, int mods)
            {
                switch(action)
                {
                    case GLFW_PRESS:
                    {
                        KeyPressedEvent e{key, false};
                        EventBus::Publish(e);
                        break;
                    }
                    case GLFW_REPEAT:
                    {
                        KeyPressedEvent e{key, true};
                        EventBus::Publish(e);
                        break;
                    }
                    case GLFW_RELEASE:
                    {
                        KeyReleasedEvent e{key};
                        EventBus::Publish(e);
                        break;
                    }
                }
            }
        );

        glfwSetMouseButtonCallback(m_WindowHandle, 
            [](GLFWwindow* window, int button, int action, int mods)
            {
                switch(action)
                {
                    case GLFW_PRESS:
                    {
                        MouseButtonPressedEvent e{button};
                        EventBus::Publish(e);
                        break;
                    }
                    case GLFW_RELEASE:
                    {
                        MouseButtonReleasedEvent e{button};
                        EventBus::Publish(e);
                        break;
                    }
                }
            }
        );

        glfwSetCursorPosCallback(m_WindowHandle, 
            [](GLFWwindow* window, double xPos, double yPos)
            {
                MouseMovedEvent e{float(xPos), float(yPos)};
                EventBus::Publish(e);
            }
        );

        glfwSetScrollCallback(m_WindowHandle, 
            [](GLFWwindow* window, double xOffset, double yOffset)
            {
                MouseScrolledEvent e{float(xOffset), float(yOffset)};
                EventBus::Publish(e);
            }
        );

        glfwSetWindowSizeCallback(m_WindowHandle, 
            [](GLFWwindow* window, int width, int height)
            {
                WindowResizedEvent e{(unsigned int)width, (unsigned int)height};
                EventBus::Publish(e);
            }
        );

        glfwSetWindowCloseCallback(m_WindowHandle, 
            [](GLFWwindow* window)
            {
                WindowClosedEvent e{};
                EventBus::Publish(e);
            }
        );
        /* ************************** */
        return true;
    }

    void Window::Shutdown()
    {
        glfwDestroyWindow(m_WindowHandle);
    }
}
