#pragma once

#include "Coust/Renderer/Vulkan/VulkanUtils.h"

namespace Coust
{
    namespace VK
    {
        class CommandBufferManager
        {
        public:
            using DrawCmd = std::function<bool(VkCommandBuffer buf, uint32_t swapchianImageIdx)>;

        public:
            bool Initialize(uint32_t frameInFlight);
            void Cleanup();

            void AddDrawCmd(DrawCmd&& cmd);

            bool Record(uint32_t frameIdx, uint32_t swapchainImageIdx);
            VkCommandBuffer GetCommandBuffer(uint32_t frameIdx);

        private:
            std::vector<VkCommandPool> m_CommandPools{};
            std::vector<VkCommandBuffer> m_CommandBuffers{};

            std::vector<DrawCmd> m_DrawCommands{};

        };

    }
}
