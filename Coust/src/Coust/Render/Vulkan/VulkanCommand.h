#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"

#include <mutex>
#include <atomic>
#include <functional>

namespace Coust::Render::VK
{
    // Command buffer is just transient object, we don't need to give it a debug name
    struct CommandBuffer
    {
        // The state enum tries to map the actual state of `VkCommandBuffer`
        // Initial:
        //          When a command buffer is allocated, it is in the initial state. Some commands are able to reset a
        //          command buffer (or a set of command buffers) back to this state from any of the executable,
        //          recording or invalid state. Command buffers in the initial state can only be moved to the
        //          recording state, or freed.
        // Recording:
        //          vkBeginCommandBuffer changes the state of a command buffer from the initial state to the
        //          recording state. Once a command buffer is in the recording state, vkCmd* commands can be used
        //          to record to the command buffer
        // Executable:
        //          vkEndCommandBuffer ends the recording of a command buffer, and moves it from the
        //          recording state to the executable state. Executable command buffers can be submitted, reset, or
        //          recorded to another command buffer.
        // Pending:
        //          Queue submission of a command buffer changes the state of a command buffer from the
        //          executable state to the pending state. Whilst in the pending state, applications must not attempt
        //          to modify the command buffer in any way - as the device may be processing the commands
        //          recorded to it. Once execution of a command buffer completes, the command buffer either
        //          reverts back to the executable state, or if it was recorded with
        //          VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, it moves to the invalid state. A synchronization
        //          command should be used to detect when this occurs.
        // Invalid:
        //          Some operations, such as modifying or deleting a resource that was used in a command
        //          recorded to a command buffer, will transition the state of that command buffer into the invalid
        //          state. Command buffers in the invalid state can only be reset or freed.
        // NonExist:
        //          Command buffer not created yet
        enum class State 
        {
            Initial,
            Recording,
            Executable,
            Pending,
            Invalid,
            NonExist,
        };

        CommandBuffer() = default;
        ~CommandBuffer() = default;

        CommandBuffer(CommandBuffer&&) = delete;
        CommandBuffer(const CommandBuffer&) = delete;
        CommandBuffer& operator=(CommandBuffer&&) = delete;
        CommandBuffer& operator=(const CommandBuffer&) = delete;

        State State = State::NonExist;

        VkFence Fence = VK_NULL_HANDLE;
        VkCommandBuffer Handle = VK_NULL_HANDLE;
    };
    
    class CommandBufferCache
    {
    public:
        using CommandBufferChangedCallback = std::function<void(const CommandBuffer&)>;

        static constexpr uint32_t COMMAND_BUFFER_COUNT = 10u;

    public:
        CommandBufferCache(const Context& ctx, bool isCompute = false) noexcept;

        ~CommandBufferCache();

        CommandBufferCache() = delete;
        CommandBufferCache(CommandBufferCache&&) = delete;
        CommandBufferCache(const CommandBufferCache&) = delete;
        CommandBufferCache& operator=(CommandBufferCache&&) = delete;
        CommandBufferCache& operator=(const CommandBufferCache&) = delete;

        // Get command buffer from cache or create a new one
        VkCommandBuffer Get() noexcept;

        // Commit current command buffer and clear all the status, if there isn't current command buffer, then do nothing.
        bool Flush() noexcept;

        // Get the finish signal for current command buffer and get rid of it from the internal dependency. 
        // It's useful for setting up the dependency between rendering and presenting, e.g. `vkQueuePresentKHR`
        // By default, the submission of the next command buffer has to wait for the previous one.
        VkSemaphore GetLastSubmissionSingal() noexcept;

        // Add new dependency for the submission of the current command buffer.
        // It's useful for calling `vkAcquireNextImageKHR`
        void InjectDependency(VkSemaphore dependency) noexcept;

        // Delete all command buffers that haven't been used for a while
        void GC() noexcept;

        // Wait for all the command buffers in the cache
        void Wait() noexcept;

        void SetCommandBufferChangedCallback(CommandBufferChangedCallback&& callback) noexcept;

    private:
        VkDevice m_Device;
        VkQueue m_Queue;

        VkCommandPool m_Pool = VK_NULL_HANDLE;

        VkSemaphore m_LastSubmissionSignal = VK_NULL_HANDLE;
        VkSemaphore m_InjectedSignal = VK_NULL_HANDLE;

        uint32_t m_CurCmdBufIdx = INVALID_IDX;
        uint32_t m_AvailableCmdBuf = COMMAND_BUFFER_COUNT;
        CommandBuffer m_AllCmdBuf[COMMAND_BUFFER_COUNT];
        VkSemaphore m_AllSubmissionSignal[COMMAND_BUFFER_COUNT];

        CommandBufferChangedCallback m_CmdBufChangedCallback;
    };
}
