#pragma once

#include "utils/Compiler.h"
#include "render/vulkan/utils/CacheSetting.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

#include <utility>

namespace coust {
namespace render {

struct VulkanCommandBuffer {
    // The state enum tries to map the actual state of `VkCommandBuffer`
    // Initial:
    //          When a command buffer is allocated, it is in the initial state.
    //          Some commands are able to reset a command buffer (or a set of
    //          command buffers) back to this state from any of the executable,
    //          recording or invalid state. Command buffers in the initial state
    //          can only be moved to the recording state, or freed.
    // Recording:
    //          vkBeginCommandBuffer changes the state of a command buffer from
    //          the initial state to the recording state. Once a command buffer
    //          is in the recording state, vkCmd* commands can be used to record
    //          to the command buffer
    // Executable:
    //          vkEndCommandBuffer ends the recording of a command buffer, and
    //          moves it from the recording state to the executable state.
    //          Executable command buffers can be submitted, reset, or recorded
    //          to another command buffer.
    // Pending:
    //          Queue submission of a command buffer changes the state of a
    //          command buffer from the executable state to the pending state.
    //          Whilst in the pending state, applications must not attempt to
    //          modify the command buffer in any way - as the device may be
    //          processing the commands recorded to it. Once execution of a
    //          command buffer completes, the command buffer either reverts back
    //          to the executable state, or if it was recorded with
    //          VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, it moves to the
    //          invalid state. A synchronization command should be used to
    //          detect when this occurs.
    // Invalid:
    //          Some operations, such as modifying or deleting a resource that
    //          was used in a command recorded to a command buffer, will
    //          transition the state of that command buffer into the invalid
    //          state. Command buffers in the invalid state can only be reset or
    //          freed.
    enum class State {
        initial,
        recording,
        executable,
        pending,
        invalid,
    };

    VulkanCommandBuffer() noexcept = default;
    ~VulkanCommandBuffer() noexcept = default;

    VulkanCommandBuffer(VulkanCommandBuffer&&) = delete;
    VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
    VulkanCommandBuffer& operator=(VulkanCommandBuffer&&) = delete;
    VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;

    State state = State::initial;

    VkFence fence = VK_NULL_HANDLE;
    VkCommandBuffer handle = VK_NULL_HANDLE;
};

class VulkanCommandBufferCache {
public:
    VulkanCommandBufferCache() = delete;
    VulkanCommandBufferCache(VulkanCommandBufferCache&&) = delete;
    VulkanCommandBufferCache(const VulkanCommandBufferCache&) = delete;
    VulkanCommandBufferCache& operator=(VulkanCommandBufferCache&&) = delete;
    VulkanCommandBufferCache& operator=(
        const VulkanCommandBufferCache&) = delete;

public:
    using CommandBufferChangedCallback =
        std::function<void(const VulkanCommandBuffer&)>;

public:
    VulkanCommandBufferCache(
        VkDevice dev, VkQueue queue, uint32_t queue_idx) noexcept;

    ~VulkanCommandBufferCache() noexcept;

    VkCommandBuffer get() noexcept;

    // Commit current command buffer and clear all the status, if there isn't
    // current command buffer, then do nothing.
    bool flush() noexcept;

    // Get the finish signal for current command buffer and get rid of it from
    // the internal dependency. It's useful for setting up the dependency
    // between rendering and presenting, e.g. `vkQueuePresentKHR` By default,
    // the submission of the next command buffer has to wait for the previous
    // one.
    VkSemaphore get_last_submission_singal() noexcept;

    // Add new dependency for the submission of the current command buffer.
    // It's useful for calling `vkAcquireNextImageKHR`
    void inject_dependency(VkSemaphore dependency) noexcept;

    // Reset all command buffers that haven't been used for a while
    void gc() noexcept;

    // Wait for all the command buffers in the cache
    void wait() noexcept;

    void set_command_buffer_changed_callback(
        CommandBufferChangedCallback&& callback) noexcept;

private:
    VkDevice m_dev;
    VkQueue m_queue;

    VkCommandPool m_pool = VK_NULL_HANDLE;

    VkSemaphore m_last_submission_signal = VK_NULL_HANDLE;
    VkSemaphore m_injected_signal = VK_NULL_HANDLE;

    std::optional<uint32_t> m_cmdbuf_idx{};
    uint32_t m_available_cmdbuf_cnt = GARBAGE_COLLECTION_PERIOD;

    std::array<VulkanCommandBuffer, GARBAGE_COLLECTION_PERIOD> m_cmdbufs{};

    std::array<VkSemaphore, GARBAGE_COLLECTION_PERIOD> m_submission_signals{};

    CommandBufferChangedCallback m_cmdbuf_changed_callback{};
};

}  // namespace render
}  // namespace coust
