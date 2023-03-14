#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"

#include <mutex>
#include <atomic>
#include <functional>

namespace Coust::Render::VK
{
    /**
     * @brief Keep track of a list of command pools, which manages a list of command buffers
     *        Typical usage is allocate a command pool for each frame when rendering 
     */
    class CommandBufferList
    {
    public:
        static constexpr uint32_t COMMAND_POOL_COUNT = 3;
        static constexpr uint32_t COMMAND_BUFFER_COUNT = 8;

    public:
        CommandBufferList() = default;
        ~CommandBufferList() = default;

        CommandBufferList(CommandBufferList&&) = delete;
        CommandBufferList(const CommandBufferList&) = delete;
        CommandBufferList& operator=(CommandBufferList&&) = delete;
        CommandBufferList& operator=(const CommandBufferList&) = delete;

        bool Init(const Context& ctx);
        void Shut();

        void Begin();

        VkCommandBuffer Get();

    private:
        struct PerPool
        {
            VkCommandPool cmdPool = VK_NULL_HANDLE;
            VkCommandBuffer cmdBuffer[COMMAND_BUFFER_COUNT]{};
            uint32_t nextAvailableBufIdx = 0;
        };

    private:
        VkDevice m_Device = VK_NULL_HANDLE;

        PerPool m_Pools[COMMAND_POOL_COUNT]{};

        uint32_t m_CurPoolIdx = 0;
    };
}
