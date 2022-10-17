#include <iostream>

#include <Coust.h>

namespace Coust
{
    class Coustol : public Application
    {
    public:
        Coustol() {}
        ~Coustol() {}
    };
    
    Application* CreateApplication()
    {
        return new Coustol();
    }
}
