#include <iostream>

#include <Coust.h>

namespace Coust
{
    class RoastApplication : public Application
    {
    public:
        RoastApplication()
        {
            std::cout << "Welcome to Coust!\n";
        }
        ~RoastApplication() {}
    };
    
    Application* CreateApplication()
    {
        return new RoastApplication();
    }
}
