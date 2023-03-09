#include "pch.h"

#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Render/Vulkan/VulkanCommand.h"
#include "Coust/Render/Vulkan/VulkanRenderPass.h"
#include "Coust/Render/Vulkan/VulkanFramebuffer.h"

namespace Coust::Render::VK
{
    CommandBuffer::CommandBuffer(CommandBuffer::ConstructParam param)
        : Base(param.commandPool.GetDevice(), VK_NULL_HANDLE), m_Level(param.level), m_CommandPoolCreatedFrom(&param.commandPool)
    {
        if (Construct(param.commandPool, param.level))
        {
            if (param.dedicatedName)
                SetDedicatedDebugName(param.dedicatedName);
            else if (param.scopeName)
                SetDefaultDebugName(param.scopeName, ToString(m_Level));
            else
                COUST_CORE_WARN("Command buffer created without a debug name");
        }
    }

    CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept
        : Base(std::forward<Base>(other)), m_Level(other.m_Level), m_CommandPoolCreatedFrom(other.m_CommandPoolCreatedFrom)
    {
        other.m_State = State::Invalid;
    }
        
    CommandBuffer::State CommandBuffer::GetState() const { return m_State; }
    
    bool CommandBuffer::IsValid() const { return m_State != State::Invalid; }

    bool CommandBuffer::Construct(const CommandPool& commandPool, VkCommandBufferLevel level)
    {
        VkCommandBufferAllocateInfo ai 
        {
            .sType                  = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool            = commandPool.GetHandle(),
            .level                  = level,
            .commandBufferCount     = 1,
        };
        VK_CHECK(vkAllocateCommandBuffers(m_Device, &ai, &m_Handle));
        m_State = State::Initial;
        return true;
    }
}
