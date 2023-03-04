#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"

namespace Coust::Render::VK
{
    class RenderPass;
    class Framebuffer;
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
        struct ConstructParam0
        {
            const CommandPool&      commandPool;
            VkCommandBufferLevel    level;
            const char*             scopeName;
        };
        /**
         * @brief Constructor with default debug name
         *
         * @param commandPool   Command pool to allocate from
         * @param level         Command buffer level
         * @param scopeName     Scope name, provided by caller
         */
        CommandBuffer(ConstructParam0 param);
        
        struct ConstructParam1
        {
            const CommandPool&      commandPool;
            VkCommandBufferLevel    level;
            const char*             dedicatedName;
        };
        /**
         * @brief Constructor with dedicated debug name
         *
         * @param commandPool       Command pool to allocate from
         * @param level             Command buffer level
         * @param dedicatedName     Dedicated name
         */
        CommandBuffer(ConstructParam1 param);

        CommandBuffer(CommandBuffer&& other);
        
        // Command buffers are always attached to command pools. 
        // And we always reset the whole command pool instead of freeing or resetting command buffer individually.
        ~CommandBuffer() = default;
        
        CommandBuffer() = delete;
        CommandBuffer(const CommandBuffer&) = delete;
        CommandBuffer& operator=(const CommandBuffer&) = delete;
        CommandBuffer& operator=(CommandBuffer&& other) = delete;
        
        State GetState() const { return m_State; }
        
        bool IsValid() const { return m_State != State::Invalid; }
    
    public:
        // All vkCmd* goes here
        
    private:
        /**
         * @brief Actual constructor, all construction happen here
         * 
         * @param commandPool
         * @param level 
         */
        void Construct(const CommandPool& commandPool, VkCommandBufferLevel level);
        
    private:
        State m_State = State::Invalid;
        
        VkCommandBufferLevel m_Level;
        
        const CommandPool* m_CommandPoolCreatedFrom;
        
        RenderPassBinding m_RenderPassBinding;
    };
    
    class CommandPool : public Resource<VkCommandPool, VK_OBJECT_TYPE_COMMAND_POOL>
    {
    public:
        using Base = Resource<VkCommandPool, VK_OBJECT_TYPE_COMMAND_POOL>;
        
    public:
        
        ~CommandPool();
        
        CommandPool(CommandPool&& other);
        CommandPool& operator=(CommandPool&& other);
            
        CommandPool() = delete;
        CommandPool(const CommandPool&) = delete;
    };
}
