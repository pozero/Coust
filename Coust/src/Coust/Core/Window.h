#pragma once

struct GLFWwindow;

namespace Coust
{
    class Window
    {
    public:
        struct Config
        {
            int width;
            int height;
            const char* name;
        };

    public:
        static Window* CreateCoustWindow(const Config& config = Config{1200, 800, "Coust Engine"});

        void Shutdown();

        int GetWidth() const { return m_WindowProp.width; }
        int GetHeight() const { return m_WindowProp.height; }

        GLFWwindow* GetHandle() const { return m_WindowHandle; }

    private:
        Window(const Config& config);
        
        bool Initialize();

    private:
        Config m_WindowProp;
        GLFWwindow* m_WindowHandle = nullptr;
    };
}
