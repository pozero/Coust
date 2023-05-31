#include "pch.h"

#include "utils/Compiler.h"
#include "render/vulkan/utils/VulkanCheck.h"
#include "render/vulkan/utils/VulkanAllocation.h"
#include "render/vulkan/VulkanCommand.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {

VulkanCommandBufferCache::VulkanCommandBufferCache(
    VkDevice dev, VkQueue queue, uint32_t queue_idx) noexcept
    : m_dev(dev), m_queue(queue) {
    VkCommandPoolCreateInfo const pool_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
                 VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = queue_idx,
    };
    COUST_VK_CHECK(vkCreateCommandPool(
                       m_dev, &pool_info, COUST_VULKAN_ALLOC_CALLBACK, &m_pool),
        "Can't create vulkan command pool");
    VkCommandBufferAllocateInfo const cmdbuf_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkSemaphoreCreateInfo const semaphore_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
    };
    VkFenceCreateInfo const fence_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };
    for (uint32_t i = 0; i < GARBAGE_COLLECTION_PERIOD; ++i) {
        COUST_VK_CHECK(
            vkAllocateCommandBuffers(m_dev, &cmdbuf_info, &m_cmdbufs[i].handle),
            "Can't create {}th vulkan command buffer", i);
        COUST_VK_CHECK(vkCreateFence(m_dev, &fence_info,
                           COUST_VULKAN_ALLOC_CALLBACK, &m_cmdbufs[i].fence),
            "Can't create {}th vulkan fence", i);
        COUST_VK_CHECK(
            vkCreateSemaphore(m_dev, &semaphore_info,
                COUST_VULKAN_ALLOC_CALLBACK, &m_submission_signals[i]),
            "Can't create {}th vulkan semaphore", i);
    }
}

VulkanCommandBufferCache::~VulkanCommandBufferCache() noexcept {
    wait();
    gc();
    vkDestroyCommandPool(m_dev, m_pool, COUST_VULKAN_ALLOC_CALLBACK);
    for (uint32_t i = 0; i < GARBAGE_COLLECTION_PERIOD; ++i) {
        vkDestroyFence(m_dev, m_cmdbufs[i].fence, COUST_VULKAN_ALLOC_CALLBACK);
    }
    for (const auto semaphore : m_submission_signals) {
        vkDestroySemaphore(m_dev, semaphore, COUST_VULKAN_ALLOC_CALLBACK);
    }
}

VkCommandBuffer VulkanCommandBufferCache::get() noexcept {
    if (m_cmdbuf_idx.has_value())
        return m_cmdbufs[m_cmdbuf_idx.value()].handle;
    while (m_available_cmdbuf_cnt == 0) {
        wait();
        gc();
    }
    for (uint32_t i = 0; i < GARBAGE_COLLECTION_PERIOD; ++i) {
        if (m_cmdbufs[i].state == VulkanCommandBuffer::State::initial) {
            m_cmdbuf_idx = i;
            --m_available_cmdbuf_cnt;
            break;
        }
    }
    VkCommandBufferBeginInfo const cmdbuf_begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    COUST_VK_CHECK(vkBeginCommandBuffer(m_cmdbufs[m_cmdbuf_idx.value()].handle,
                       &cmdbuf_begin_info),
        "");
    m_cmdbufs[m_cmdbuf_idx.value()].state =
        VulkanCommandBuffer::State::recording;
    return m_cmdbufs[m_cmdbuf_idx.value()].handle;
}

bool VulkanCommandBufferCache::flush() noexcept {
    if (!m_cmdbuf_idx.has_value())
        return false;
    auto& cmdbuf = m_cmdbufs[m_cmdbuf_idx.value()];
    COUST_VK_CHECK(vkEndCommandBuffer(cmdbuf.handle),
        "Can't end {} th vulkan command buffer", m_cmdbuf_idx.value());
    cmdbuf.state = VulkanCommandBuffer::State::executable;
    std::array<VkPipelineStageFlags, 2> wait_stages{
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    };
    std::array<VkSemaphore, 2> wait_signals{
        VK_NULL_HANDLE,
        VK_NULL_HANDLE,
    };
    VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = wait_signals.data(),
        .pWaitDstStageMask = wait_stages.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdbuf.handle,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &m_submission_signals[m_cmdbuf_idx.value()],
    };
    if (m_last_submission_signal != VK_NULL_HANDLE) {
        wait_signals[submit_info.waitSemaphoreCount++] =
            m_last_submission_signal;
    }
    if (m_injected_signal != VK_NULL_HANDLE) {
        wait_signals[submit_info.waitSemaphoreCount++] = m_injected_signal;
    }
    COUST_VK_CHECK(vkQueueSubmit(m_queue, 1, &submit_info, cmdbuf.fence),
        "Can't sumbit {} th vulkan command buffer", m_cmdbuf_idx.value());
    cmdbuf.state = VulkanCommandBuffer::State::pending;
    m_last_submission_signal = m_submission_signals[m_cmdbuf_idx.value()];
    m_injected_signal = VK_NULL_HANDLE;
    m_cmdbuf_idx.reset();
    if (m_cmdbuf_changed_callback != nullptr) {
        m_cmdbuf_changed_callback(cmdbuf);
    }
    return true;
}

VkSemaphore VulkanCommandBufferCache::get_last_submission_singal() noexcept {
    VkSemaphore ret = VK_NULL_HANDLE;
    std::swap(ret, m_last_submission_signal);
    return ret;
}

void VulkanCommandBufferCache::inject_dependency(
    VkSemaphore dependency) noexcept {
    m_injected_signal = dependency;
}

void VulkanCommandBufferCache::gc() noexcept {
    for (uint32_t i = 0; i < GARBAGE_COLLECTION_PERIOD; ++i) {
        auto& cmdbuf = m_cmdbufs[i];
        VkResult const res = vkGetFenceStatus(m_dev, cmdbuf.fence);
        if (res == VK_SUCCESS) {
            vkResetFences(m_dev, 1, &cmdbuf.fence);
            cmdbuf.state = VulkanCommandBuffer::State::invalid;
            ++m_available_cmdbuf_cnt;
        }
    }
    bool const can_reset_pool =
        std::ranges::all_of(m_cmdbufs, [](VulkanCommandBuffer const& cmdbuf) {
            return cmdbuf.state == VulkanCommandBuffer::State::initial ||
                   cmdbuf.state == VulkanCommandBuffer::State::invalid;
        });
    if (can_reset_pool) {
        COUST_VK_CHECK(vkResetCommandPool(m_dev, m_pool, 0), "");
        std::ranges::for_each(m_cmdbufs, [](VulkanCommandBuffer& cmdbuf) {
            cmdbuf.state = VulkanCommandBuffer::State::initial;
        });
    } else {
        for (auto& cmdbuf : m_cmdbufs) {
            if (cmdbuf.state == VulkanCommandBuffer::State::invalid) {
                COUST_VK_CHECK(vkResetCommandBuffer(cmdbuf.handle, 0), "");
                cmdbuf.state = VulkanCommandBuffer::State::initial;
            }
        }
    }
}

void VulkanCommandBufferCache::wait() noexcept {
    std::array<VkFence, GARBAGE_COLLECTION_PERIOD> fences_to_wait{};
    uint32_t idx = 0;
    for (uint32_t i = 0; i < GARBAGE_COLLECTION_PERIOD; ++i) {
        if (m_cmdbuf_idx.has_value() && i == m_cmdbuf_idx.value())
            continue;
        fences_to_wait[idx++] = m_cmdbufs[i].fence;
        m_cmdbufs[i].state = VulkanCommandBuffer::State::invalid;
    }
    if (idx > 0)
        COUST_VK_CHECK(vkWaitForFences(m_dev, idx, fences_to_wait.data(),
                           VK_TRUE, std::numeric_limits<uint64_t>::max()),
            "");
}

void VulkanCommandBufferCache::set_command_buffer_changed_callback(
    CommandBufferChangedCallback&& callback) noexcept {
    m_cmdbuf_changed_callback = std::move(callback);
}

}  // namespace render
}  // namespace coust
