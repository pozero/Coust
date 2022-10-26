#include "pch.h"

#include "Window.h"
#include "Application.h"
#include "Event/KeyEvent.h"
#include "Event/MouseEvent.h"
#include "Event/ApplicationEvent.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Coust
{
    bool Window::Input::IsKeyDown(int keyCode)
    {
        GLFWwindow* windowHandle = Application::GetInstance()->GetWindow()->GetWindowHandle();

        return !(glfwGetKey(windowHandle, keyCode) == GLFW_RELEASE);
    }

    bool Window::Input::IsMouseButtonDown(int button)
    {
        GLFWwindow* windowHandle = Application::GetInstance()->GetWindow()->GetWindowHandle();

        return !(glfwGetMouseButton(windowHandle, button) == GLFW_RELEASE);
    }

    std::pair<float, float> Window::Input::GetCursorPos()
    {
        double x, y;
        GLFWwindow* windowHandle = Application::GetInstance()->GetWindow()->GetWindowHandle();

        glfwGetCursorPos(windowHandle, &x, &y);
        return std::pair<float, float>{float(x), float(y)};
    }

    float Window::Input::GetCursorPosX()
    {
        auto[x, y] = Window::Input::GetCursorPos();
        return x;
    }
    float Window::Input::GetCursorPosY()
    {
        auto[x, y] = Window::Input::GetCursorPos();
        return y;
    }

    Window::Window(const Config& config)
    {
        Init(config);
    }

    Window::~Window()
    {
        Shutdown();
    }

    static void GLFWErrorCallback(int error, const char* desc)
    {
        COUST_CORE_ERROR("GLFW Error ({0}): {1}", error, desc);
    }

    void Window::Init(const Config& config)
    {
        m_WindowProp = config;

        int glfwInitializationSuccess = glfwInit();
        COUST_CORE_ASSERT(glfwInitializationSuccess, "GLFW Initialization Falied");

        glfwSetErrorCallback(GLFWErrorCallback);

        m_WindowHandle = glfwCreateWindow(config.width, config.height, config.name, nullptr, nullptr);
        glfwMakeContextCurrent(m_WindowHandle);

        int gladInitializationSuccess = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        COUST_CORE_ASSERT(gladInitializationSuccess, "GLAD Initialization Failed");

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
    }

    void Window::Shutdown()
    {
        glfwDestroyWindow(m_WindowHandle);
    }

    void Window::OnUpdate()
    {
        glfwPollEvents();
        glfwSwapBuffers(m_WindowHandle);
    }
}