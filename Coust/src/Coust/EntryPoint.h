#pragma once

extern Coust::Application* Coust::CreateApplication();

int main(int argc, char* argv[])
{
    bool globalContextInitialized = Coust::Logger::Initialize();

    if (globalContextInitialized)
    {
        COUST_CORE_INFO("Welcome to Coust!");
        auto app = Coust::CreateApplication();
        app->Run();
        delete app;
        Coust::Logger::Shutdown();
    }
    else
        COUST_CORE_ERROR("Global Context Initialization Failed");

    return 0;
}