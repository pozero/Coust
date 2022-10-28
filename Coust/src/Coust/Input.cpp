#include <pch.h>

#include "Input.h"
#include "Window.h"
#include "Application.h"

#include <GLFW/glfw3.h>

namespace Coust
{
    bool Input::IsKeyDown(int keyCode)
    {
        GLFWwindow* windowHandle = Application::GetInstance().GetWindow().GetWindowHandle();

        return !(glfwGetKey(windowHandle, keyCode) == GLFW_RELEASE);
    }

    bool Input::IsMouseButtonDown(int button)
    {
        GLFWwindow* windowHandle = Application::GetInstance().GetWindow().GetWindowHandle();

        return !(glfwGetMouseButton(windowHandle, button) == GLFW_RELEASE);
    }

    std::pair<float, float> Input::GetCursorPos()
    {
        double x, y;
        GLFWwindow* windowHandle = Application::GetInstance().GetWindow().GetWindowHandle();

        glfwGetCursorPos(windowHandle, &x, &y);
        return std::pair<float, float>{float(x), float(y)};
    }

    float Input::GetCursorPosX()
    {
        auto[x, y] = GetCursorPos();
        return x;
    }
    float Input::GetCursorPosY()
    {
        auto[x, y] = GetCursorPos();
        return y;
    }
}