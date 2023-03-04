#include "pch.h"

#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Render/Vulkan/VulkanCommand.h"
#include "Coust/Render/Vulkan/VulkanRenderPass.h"
#include "Coust/Render/Vulkan/VulkanFramebuffer.h"

namespace Coust::Render::VK
{
    CommandBuffer::CommandBuffer(CommandBuffer::ConstructParam0 param)
        : Base(param.commandPool.GetDevice(), VK_NULL_HANDLE), m_Level(param.level), m_CommandPoolCreatedFrom(&param.commandPool)
    {
        Construct(param.commandPool, param.level);
        if (m_State == State::Initial)
        {
            SetDefaultDebugName(param.scopeName, ToString(m_Level));
        }
    }

    CommandBuffer::CommandBuffer(CommandBuffer::ConstructParam1 param)
        : Base(param.commandPool.GetDevice(), VK_NULL_HANDLE), m_Level(param.level), m_CommandPoolCreatedFrom(&param.commandPool)
    {
        Construct(param.commandPool, param.level);
        if (m_State == State::Initial)
        {
            SetDedicatedDebugName(param.dedicatedName);
        }
    }
    
    CommandBuffer::CommandBuffer(CommandBuffer&& other)
        : Base(std::forward<Base>(other)), m_Level(other.m_Level), m_CommandPoolCreatedFrom(other.m_CommandPoolCreatedFrom)
    {
        other.m_State = State::Invalid;
    }

    void CommandBuffer::Construct(const CommandPool& commandPool, VkCommandBufferLevel level)
    {
        VkCommandBufferAllocateInfo ai 
        {
            .sType                  = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool            = commandPool.GetHandle(),
            .level                  = level,
            .commandBufferCount     = 1,
        };
        bool succeeded = false;
        VK_REPORT(vkAllocateCommandBuffers(m_Device, &ai, &m_Handle), succeeded);
        if (succeeded)
            m_State = State::Initial;
    }
}
