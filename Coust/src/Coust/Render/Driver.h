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
        virtual void Shutdown() = 0;

        virtual bool RecreateSwapchainAndFramebuffers() = 0;

        virtual bool FlushCommand() = 0;

    private:
        virtual bool Initialize() = 0;
        
    public:
        static Driver* CreateDriver(API api);
    };
}