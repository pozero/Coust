#include <pch.h>

#include "Coust/Core/Input.h"
#include "Coust/Core/Window.h"
#include "Coust/Core/Application.h"

#include <GLFW/glfw3.h>

namespace Coust
{
    extern GLFWwindow* g_WindowHandle;
    bool Input::IsKeyDown(int keyCode)
    {
        return !(glfwGetKey(g_WindowHandle, keyCode) == GLFW_RELEASE);
    }

    bool Input::IsMouseButtonDown(int button)
    {
        return !(glfwGetMouseButton(g_WindowHandle, button) == GLFW_RELEASE);
    }

    std::pair<float, float> Input::GetCursorPos()
    {
        double x, y;
        glfwGetCursorPos(g_WindowHandle, &x, &y);
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
