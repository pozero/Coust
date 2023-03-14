#pragma once

#include <thread>
#include <mutex>

namespace Coust::Render
{
    enum class API
    {
        VULKAN
    };

    class Driver 
    {
    public:
        virtual ~Driver() {}
        
        virtual void InitializationTest() {};

        virtual void LoopTest() {};

        static Driver* CreateDriver(API api);
        
        bool IsInitialized() const { return m_IsInitialized; }
    
    protected:
        bool m_IsInitialized = false;
    };
}