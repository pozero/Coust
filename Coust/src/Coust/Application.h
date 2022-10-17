#pragma once

#include "Api.h"

namespace Coust
{
    class COUST_API Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();
    };

    Application* CreateApplication();
}