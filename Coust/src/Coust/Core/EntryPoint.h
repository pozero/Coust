#pragma once

extern Coust::Application* Coust::CreateApplication();

static bool AllSystemInit()
{
    if (!Coust::Logger::Initialize())
    {
        std::cerr << "Logger Initialization Failed\n";
        return false;
    }

    if (!Coust::Window::Init())
    {
        COUST_CORE_ERROR("Window Initialization Failed");
        return false;
    }
    
    if (!Coust::RenderBackend::Init())
    {
        COUST_CORE_ERROR("Vulkan Backend Initialization Failed");
        return false;
    }
    return true;
}

static void AllSystemShut()
{
    Coust::RenderBackend::Shut();
    Coust::Window::Shut();
    Coust::Logger::Shutdown();
}

int main(int argc, char* argv[])
{
    do
    {
        if (AllSystemInit())
        {
            auto app = Coust::CreateApplication();
            COUST_CORE_INFO("Welcome to Coust!");
            app->Run();
            delete app;
        }
    } while (false);

    AllSystemShut();

    return 0;
}