#pragma once

extern Coust::Application* Coust::CreateApplication();


int main(int argc, char* argv[])
{
    do
    {
        Coust::GlobalContext context{};
        if (context.Initialize())
        {
            auto app = Coust::CreateApplication();
            COUST_CORE_INFO("Welcome to Coust!");
            app->Run();
            delete app;
        }
        context.Shutdown();
    } while (false);

    return 0;
}