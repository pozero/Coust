#pragma once

#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Render/Vulkan/VulkanContext.h"
#include "Coust/Render/Vulkan/VulkanRenderPass.h"
#include "Coust/Render/Vulkan/VulkanFramebuffer.h"

namespace Coust::Render::VK
{
    class CommandBUffer;
    class CommandPool;

    /**
     * @brief Wrapping of all vkCmd*() function.
     *        Also tracks the pipeline state and resource (image, buffer) binding state.
     */
    class CommandBuffer : public Resource<VkCommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER>
    {
    public:
        using Base = Resource<VkCommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER>;

        enum class State 
        {
            Initial,
            Recording,
            Executable,
            Pending,
            Invalid,
        };
        
        /**
         * @brief Helper struct keeps track of target `RenderPass` and `Framebuffer`
         */
        struct RenderPassBinding
        {
            const RenderPass* renderPass;
            const Framebuffer* framebuffer;
        };
        
    public:
        CommandBuffer() = delete;
        CommandBuffer(const CommandBuffer&) = delete;
        CommandBuffer& operator=(const CommandBuffer&) = delete;
        CommandBuffer& operator=(CommandBuffer&&) = delete;
        
        /**
         * @brief Constructor with default debug name
         *
         * @param commandPool   Command pool to allocate from
         * @param level         Command buffer level
         * @param scopeName     Scope name, provided by caller
         */
        CommandBuffer(const CommandPool& commandPool, VkCommandBufferLevel level, const char* scopeName);
        
        /**
         * @brief Constructor with dedicated debug name
         *
         * @param commandPool   Command pool to allocate from
         * @param level         Command buffer level
         * @param debugName     Dedicated name
         */
        CommandBuffer(const CommandPool& commandPool, VkCommandBufferLevel level, std::string&& debugName);

        CommandBuffer(CommandBuffer&& other);
        
        // Command buffers are always attached to command pools. 
        // And we always reset the whole command pool instead of freeing or resetting command buffer individually.
        ~CommandBuffer() = default;
        
        State GetState() const { return m_State; }
        
        bool IsValid() const { return m_State != State::Invalid; }
    
    public:
        // All vkCmd* goes here
        
        /**
         * @brief Wrapper of `vkCmdBegin`
         * 
         * @param flags 
         * @param primaryCommandBuffer      Optional. Required if the command buffer is a secondary command buffer.
         * @return VkResult 
         */
        VkResult Begin(VkCommandBufferUsageFlags flags, const CommandBuffer* primaryCommandBuffer);

        /**
         * @brief Wrapper of `vkCmdBegin`
         * 
         * @param flags 
         * @param renderPassBinding         Optional. Required if the command buffer is a secondary command buffer. 
         * @param subpass                   Optional.
         * @return VkResult 
         */
        VkResult Begin(VkCommandBufferUsageFlags flags, const RenderPassBinding& renderPassBinding, uint32_t subpass);
        
    private:
        /**
         * @brief Actual constructor, all construction happens here
         * 
         * @param commandPool
         * @param level 
         */
        void Construct(const CommandPool& commandPool, VkCommandBufferLevel level);
        
    private:
        State m_State = State::Invalid;
        
        VkCommandBufferLevel m_Level;
        
        CommandPool& m_CommandPoolCreatedFrom;
        
        RenderPassBinding m_RenderPassBinding;
    };
    
    class CommandPool : public Resource<VkCommandPool, VK_OBJECT_TYPE_COMMAND_POOL>
    {
        
    };
}
