#pragma once

extern Coust::Application* Coust::CreateApplication();


int main(int argc, char* argv[])
{
    do
    {
        if (Coust::AllSystemInit())
        {
            auto app = Coust::CreateApplication();
            COUST_CORE_INFO("Welcome to Coust!");
            app->Run();
            delete app;
        }
    } while (false);

    Coust::AllSystemShut();

    return 0;
}