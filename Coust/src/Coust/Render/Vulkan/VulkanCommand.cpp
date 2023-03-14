#include "pch.h"

#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Render/Vulkan/VulkanCommand.h"

namespace Coust::Render::VK
{
    bool CommandBufferList::Init(const Context& ctx)
    {
        m_Device = ctx.Device;

        VkCommandPoolCreateInfo cmdPoolCI 
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = ctx.GraphicsQueueFamilyIndex,
        };

        VkCommandBufferAllocateInfo bufAI 
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = VK_NULL_HANDLE,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = COMMAND_BUFFER_COUNT,
        };

        for (uint32_t p = 0; p < COMMAND_POOL_COUNT; ++ p)
        {
            VK_CHECK(vkCreateCommandPool(m_Device, &cmdPoolCI, nullptr, &m_Pools[p].cmdPool));

            bufAI.commandPool = m_Pools[p].cmdPool;
            VK_CHECK(vkAllocateCommandBuffers(m_Device, &bufAI, m_Pools[p].cmdBuffer));
        }

        return true;
    }

    void CommandBufferList::Shut()
    {
        for (uint32_t p = 0; p < COMMAND_POOL_COUNT; ++ p)
        {
            vkFreeCommandBuffers(m_Device, m_Pools[p].cmdPool, COMMAND_BUFFER_COUNT, &m_Pools[p].cmdBuffer[0]);
            vkDestroyCommandPool(m_Device, m_Pools[p].cmdPool, nullptr);
        }
    }

    void CommandBufferList::Begin()
    {
        m_CurPoolIdx = (++m_CurPoolIdx) % COMMAND_POOL_COUNT;
        m_Pools[m_CurPoolIdx].nextAvailableBufIdx = 0;
    }

    VkCommandBuffer CommandBufferList::Get()
    {
        if (m_Pools[m_CurPoolIdx].nextAvailableBufIdx >= COMMAND_BUFFER_COUNT)
        {
            // If there isn't enough free command buffer, just log and return null
            COUST_CORE_ERROR("There isn't enough command buffer available. Consider increase the `COMMAND_BUFFER_COUNT`");
            return VK_NULL_HANDLE;
        }

        return m_Pools[m_CurPoolIdx].cmdBuffer[m_Pools[m_CurPoolIdx].nextAvailableBufIdx++];
    }
}
