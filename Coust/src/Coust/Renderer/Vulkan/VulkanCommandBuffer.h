#pragma once

#include "Coust/Renderer/Vulkan/VulkanUtils.h"
#include "vulkan/vulkan_core.h"

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

            bool RecordDrawCmd(uint32_t frameIdx, uint32_t swapchainImageIdx);
            VkCommandBuffer GetCommandBuffer(uint32_t frameIdx);

        private:
            std::vector<VkCommandPool> m_DrawCommandPools{};
            std::vector<VkCommandBuffer> m_DrawCommandBuffers{};

            std::vector<DrawCmd> m_DrawCommands{};

            VkCommandPool m_UploadCommandPools = VK_NULL_HANDLE;

        };

    }
}
