#include "pch.h"

#include "Coust/Renderer/Vulkan/VulkanCommandBuffer.h"

namespace Coust
{
    namespace VK 
    {
        bool CommandBufferManager::Initialize(uint32_t frameInFlight)
        {
            m_CommandPools.resize(frameInFlight);
            m_CommandBuffers.resize(frameInFlight);
            for (uint32_t i = 0; i < frameInFlight; ++i)
            {
                VkCommandPoolCreateInfo poolInfo
                {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                    .queueFamilyIndex = g_GraphicsQueueFamilyIndex,
                };
                VK_CHECK(vkCreateCommandPool(g_Device, &poolInfo, nullptr, &m_CommandPools[i]));

                VkCommandBufferAllocateInfo bufInfo
                {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                    .commandPool = m_CommandPools[i],
                    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                    .commandBufferCount = 1,
                };
                VK_CHECK(vkAllocateCommandBuffers(g_Device, &bufInfo, &m_CommandBuffers[i]));
            }

            return true;
        }

        void CommandBufferManager::Cleanup()
        {
            vkDeviceWaitIdle(g_Device);
            for (VkCommandPool p : m_CommandPools)
            {
                vkDestroyCommandPool(g_Device, p, nullptr);
            }
        }

        void CommandBufferManager::AddDrawCmd(DrawCmd&& cmd)
        {
            m_DrawCommands.push_back(cmd);
        }

        bool CommandBufferManager::Record(uint32_t frameIdx, uint32_t swapchainImageIdx)
        {
            vkResetCommandBuffer(m_CommandBuffers[frameIdx], 0u);

            for (const auto& cmd : m_DrawCommands)
            {
                if (!cmd(m_CommandBuffers[frameIdx], swapchainImageIdx))
                    return false;
            }
            return true;
        }

        VkCommandBuffer CommandBufferManager::GetCommandBuffer(uint32_t frameIdx)
        {
            return m_CommandBuffers[frameIdx];
        }
    }
}
