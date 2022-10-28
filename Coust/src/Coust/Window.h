#pragma once

struct GLFWwindow;

namespace Coust
{
    class Window
    {
    public:
        struct Config
        {
            int width = 1200;
            int height = 800;
            const char* name = "Coust Engine";
        };

    public:
        Window(const Config& config = Config{});
        ~Window();
        
        void Init(const Config& config);
        void Shutdown();

        void OnUpdate();

        GLFWwindow* GetWindowHandle() const { return m_WindowHandle; }

        int GetWidth() const { return m_WindowProp.width; }
        int GetHeight() const { return m_WindowProp.height; }

    private:
        Config m_WindowProp;
        GLFWwindow* m_WindowHandle;
    };
}