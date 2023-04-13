#include "pch.h"

#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Render/Vulkan/VulkanCommand.h"

namespace Coust::Render::VK
{
        CommandBufferCache::CommandBufferCache(const Context& ctx, bool isCompute) noexcept
            : m_Device(ctx.Device), m_Queue(isCompute ? ctx.ComputeQueue : ctx.GraphicsQueue)
        {
            VkCommandPoolCreateInfo poolCI 
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                .queueFamilyIndex = isCompute ? ctx.ComputeQueueFamilyIndex : ctx.GraphicsQueueFamilyIndex,
            };
            VK_CHECK(vkCreateCommandPool(m_Device, &poolCI, nullptr, &m_Pool), "Can't create command pool");

            VkSemaphoreCreateInfo semaphoreCI { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            for (uint32_t i = 0; i < COMMAND_BUFFER_COUNT; ++ i)
            {
                VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCI, nullptr, &m_AllSubmissionSignal[i]), "Can't create command buffer submission signal semaphore");
            }
        }

        CommandBufferCache::~CommandBufferCache() noexcept
        {
            Wait();
            GC();
            vkDestroyCommandPool(m_Device, m_Pool, nullptr);
            for (auto semaphore : m_AllSubmissionSignal)
            {
                vkDestroySemaphore(m_Device, semaphore, nullptr);
            }
        }

        VkCommandBuffer CommandBufferCache::Get() noexcept
        {
            if (m_CurCmdBufIdx != INVALID_IDX)
            {
                return m_AllCmdBuf[m_CurCmdBufIdx].Handle;
            }

            while (m_AvailableCmdBuf == 0)
            {
                Wait();
                GC();
            }

            for (uint32_t i = 0; i < COMMAND_BUFFER_COUNT; ++ i)
            {
                if (m_AllCmdBuf[i].State == CommandBuffer::State::NonExist)
                {
                    m_CurCmdBufIdx = i;
                    -- m_AvailableCmdBuf;
                    break;
                }
            }

            VkCommandBufferAllocateInfo cmdBufAI 
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = m_Pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };
            VK_CHECK(vkAllocateCommandBuffers(m_Device, &cmdBufAI, &m_AllCmdBuf[m_CurCmdBufIdx].Handle), "Can't allocate new command buffer");

            VkFenceCreateInfo fenceCI { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, };
            VK_CHECK(vkCreateFence(m_Device, &fenceCI, nullptr, &m_AllCmdBuf[m_CurCmdBufIdx].Fence), "Can't create fence attached to command buffer");

            m_AllCmdBuf[m_CurCmdBufIdx].State = CommandBuffer::State::Initial;

            VkCommandBufferBeginInfo cmdBufBI 
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            };
            VK_CHECK(vkBeginCommandBuffer(m_AllCmdBuf[m_CurCmdBufIdx].Handle, &cmdBufBI), "Can't begin command buffer");

            m_AllCmdBuf[m_CurCmdBufIdx].State = CommandBuffer::State::Recording;

            return m_AllCmdBuf[m_CurCmdBufIdx].Handle;
        }

        bool CommandBufferCache::Flush() noexcept
        {
            if (m_CurCmdBufIdx == INVALID_IDX)
                return false;

            vkEndCommandBuffer(m_AllCmdBuf[m_CurCmdBufIdx].Handle);
            m_AllCmdBuf[m_CurCmdBufIdx].State = CommandBuffer::State::Executable;

            // simply wait for all
            VkPipelineStageFlags waitStages[2] 
            {
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            };

            VkSemaphore waitSignals[2]
            {
                VK_NULL_HANDLE,
                VK_NULL_HANDLE,
            };

            VkSubmitInfo SI
            {   
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount = 0,
                .pWaitSemaphores = waitSignals,
                .pWaitDstStageMask = waitStages,
                .commandBufferCount = 1,
                .pCommandBuffers = &m_AllCmdBuf[m_CurCmdBufIdx].Handle,
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &m_AllSubmissionSignal[m_CurCmdBufIdx],
            };

            if (m_LastSubmissionSignal != VK_NULL_HANDLE)
                waitSignals[SI.waitSemaphoreCount++] = m_LastSubmissionSignal;

            if (m_InjectedSignal != VK_NULL_HANDLE)
                waitSignals[SI.waitSemaphoreCount++] = m_InjectedSignal;

            VK_CHECK(vkQueueSubmit(m_Queue, 1, &SI, m_AllCmdBuf[m_CurCmdBufIdx].Fence), "Can't submit / flush command buffer");
            m_AllCmdBuf[m_CurCmdBufIdx].State = CommandBuffer::State::Pending;

            m_LastSubmissionSignal = m_AllSubmissionSignal[m_CurCmdBufIdx];
            m_InjectedSignal = VK_NULL_HANDLE;
            m_CurCmdBufIdx = INVALID_IDX;

            if (m_CmdBufChangedCallback != nullptr)
                m_CmdBufChangedCallback(m_AllCmdBuf[m_CurCmdBufIdx]);

            return true;
        }

        VkSemaphore CommandBufferCache::GetLastSubmissionSingal() noexcept
        {
            VkSemaphore res = VK_NULL_HANDLE;
            std::swap(res, m_LastSubmissionSignal);
            return res;
        }

        void CommandBufferCache::InjectDependency(VkSemaphore dependency) noexcept
        {
            m_InjectedSignal = dependency;
        }

        void CommandBufferCache::GC() noexcept
        {
            for (uint32_t i = 0; i < COMMAND_BUFFER_COUNT; ++ i)
            {
                if (m_AllCmdBuf[i].State != CommandBuffer::State::NonExist)
                {
                    // time out is 0, just query the status of fence (execution finished or not in other words)
                    VkResult res = vkWaitForFences(m_Device, 1, &m_AllCmdBuf[i].Fence, VK_TRUE, 0);
                    if (res == VK_SUCCESS)
                    {
                        vkFreeCommandBuffers(m_Device, m_Pool, 1, &m_AllCmdBuf[i].Handle);
                        m_AllCmdBuf[i].Handle = VK_NULL_HANDLE;
                        vkDestroyFence(m_Device, m_AllCmdBuf[i].Fence, nullptr);
                        m_AllCmdBuf[i].Fence = VK_NULL_HANDLE;
                        m_AllCmdBuf[i].State = CommandBuffer::State::NonExist;

                        ++ m_AvailableCmdBuf;
                    }
                }
            }
        }

        void CommandBufferCache::Wait() noexcept
        {
            VkFence fencesToWait[COMMAND_BUFFER_COUNT];
            uint32_t idx = 0;
            for (uint32_t i = 0; i < COMMAND_BUFFER_COUNT; ++ i)
            {
                // the current command buffer might not even be submitted, skip it
                if (i != m_CurCmdBufIdx && m_AllCmdBuf[i].State != CommandBuffer::State::NonExist)
                {
                    fencesToWait[idx++] = m_AllCmdBuf[i].Fence;
                    m_AllCmdBuf[i].State = CommandBuffer::State::Invalid;
                }
            }
            if (idx > 0)
                vkWaitForFences(m_Device, idx, fencesToWait, VK_TRUE, UINT64_MAX);
        }

        void CommandBufferCache::SetCommandBufferChangedCallback(CommandBufferChangedCallback&& callback) noexcept
        {
            m_CmdBufChangedCallback = callback;
        }
}
