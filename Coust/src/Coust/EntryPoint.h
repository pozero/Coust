#pragma once

extern Coust::Application* Coust::CreateApplication();

int main(int argc, char* argv[])
{
    auto app = Coust::CreateApplication();
    app->Run();
    delete app;
    return 0;
}