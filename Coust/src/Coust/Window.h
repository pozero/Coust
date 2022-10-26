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

        class Input
        {
        public:
            static bool IsKeyDown(int keyCode);

            static bool IsMouseButtonDown(int button);

            static std::pair<float, float> GetCursorPos();

            static float GetCursorPosX();
            static float GetCursorPosY();
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