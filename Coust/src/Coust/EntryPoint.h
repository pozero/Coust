#pragma once

extern Coust::Application* Coust::CreateApplication();

int main(int argc, char* argv[])
{
    Coust::Logger::Initialize();
    COUST_CORE_INFO("Welcome to Coust!");

    auto app = Coust::CreateApplication();
    app->Run();
    delete app;

    Coust::Logger::Shutdown();
    return 0;
}