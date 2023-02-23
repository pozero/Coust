#pragma once

#include "Coust/Render/Vulkan/VulkanUtils.h"

namespace Coust::Render::VK
{
    class CommandBufferManager
    {
    public:
        using DrawCmd = std::function<bool(VkCommandBuffer buf, uint32_t swapchianImageIdx)>;

    public:
        bool Initialize(const Context &ctx, uint32_t frameInFlight);
        void Cleanup(const Context &ctx);

        void AddDrawCmd(DrawCmd&& cmd);

        bool RecordDrawCmd(uint32_t frameIdx, uint32_t swapchainImageIdx);
        VkCommandBuffer GetCommandBuffer(uint32_t frameIdx);
        
    private:
        std::vector<VkCommandPool> m_DrawCommandPools{};
        std::vector<VkCommandBuffer> m_DrawCommandBuffers{};

        std::vector<DrawCmd> m_DrawCommands{};
    };
}
