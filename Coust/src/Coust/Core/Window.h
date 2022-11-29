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
        [[nodiscard]] static bool Init();
        static void Shut();
        
        static Window* s_Window;

    private:
        Window(const Config& config = Config{1200, 800, "Coust Engine"});
        
        bool Initialize();
        void Shutdown();

        int GetWidth() const { return m_WindowProp.width; }
        int GetHeight() const { return m_WindowProp.height; }

        GLFWwindow* GetHandle() const { return m_WindowHandle; }

    private:
        Config m_WindowProp;
        GLFWwindow* m_WindowHandle = nullptr;
    };
}
