#include "pch.h"

#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Render/Vulkan/VulkanCommand.h"

namespace Coust::Render::VK
{
        CommandBufferCache::CommandBufferCache(const Context& ctx, bool isCompute)
            : m_Device(ctx.Device), m_Queue(isCompute ? ctx.ComputeQueue : ctx.GraphicsQueue)   
        {
            VkCommandPoolCreateInfo poolCI 
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                .queueFamilyIndex = isCompute ? ctx.ComputeQueueFamilyIndex : ctx.GraphicsQueueFamilyIndex,
            };
            bool success = false; (void) success;
            VK_REPORT(vkCreateCommandPool(m_Device, &poolCI, nullptr, &m_Pool), success);

            VkSemaphoreCreateInfo semaphoreCI { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            for (uint32_t i = 0; i < COMMAND_BUFFER_COUNT; ++ i)
            {
                VK_REPORT(vkCreateSemaphore(m_Device, &semaphoreCI, nullptr, &m_AllSubmissionSignal[i]), success);
            }
        }

        CommandBufferCache::~CommandBufferCache()
        {
            Wait();
            GC();
            vkDestroyCommandPool(m_Device, m_Pool, nullptr);
            for (auto semaphore : m_AllSubmissionSignal)
            {
                vkDestroySemaphore(m_Device, semaphore, nullptr);
            }
        }

        const CommandBuffer* CommandBufferCache::Get()
        {
            if (m_CurCmdBufIdx != INVALID_IDX)
            {
                return &m_AllCmdBuf[m_CurCmdBufIdx];
            }

            while (m_AvailableCmdBuf == 0)
            {
                COUST_CORE_TRACE("No available command buffer, command cache stalled. If this happens frequently, consider increasing `COMMAND_BUFFER_COUNT`");
                Wait();
                GC();
            }

            for (uint32_t i = 0; i < COMMAND_BUFFER_COUNT; ++ i)
            {
                if (m_AllCmdBuf[i].State.load(std::memory_order::seq_cst) == CommandBuffer::State::NonExist)
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
            bool success = false;
            VK_REPORT(vkAllocateCommandBuffers(m_Device, &cmdBufAI, &m_AllCmdBuf[m_CurCmdBufIdx].Handle), success);

            if (!success)
            {
                COUST_CORE_ERROR("Can't allocate new command buffer, return empty buffer");
                m_AllCmdBuf[m_CurCmdBufIdx].Handle = VK_NULL_HANDLE;
                return nullptr;
            }

            VkFenceCreateInfo fenceCI { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, };
            // The only two senario these vulkan call would fail is we've run out of host or device memory, which is really rare. 
            // And if it actually happened, the allocation of command buffer probably already failed.
            VK_REPORT(vkCreateFence(m_Device, &fenceCI, nullptr, &m_AllCmdBuf[m_CurCmdBufIdx].Fence), success);

            m_AllCmdBuf[m_CurCmdBufIdx].State.store(CommandBuffer::State::Initial);

            VkCommandBufferBeginInfo cmdBufBI 
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            };
            VK_REPORT(vkBeginCommandBuffer(m_AllCmdBuf[m_CurCmdBufIdx].Handle, &cmdBufBI), success);

            m_AllCmdBuf[m_CurCmdBufIdx].State.store(CommandBuffer::State::Recording);

            COUST_CORE_TRACE("Get command buffer {}", m_CurCmdBufIdx);
            return &m_AllCmdBuf[m_CurCmdBufIdx];
        }

        bool CommandBufferCache::Flush()
        {
            if (m_CurCmdBufIdx == INVALID_IDX)
                return false;

            vkEndCommandBuffer(m_AllCmdBuf[m_CurCmdBufIdx].Handle);
            m_AllCmdBuf[m_CurCmdBufIdx].State.store(CommandBuffer::State::Executable, std::memory_order::relaxed);

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

            VK_CHECK(vkQueueSubmit(m_Queue, 1, &SI, m_AllCmdBuf[m_CurCmdBufIdx].Fence));

            COUST_CORE_TRACE("Flush command buffer {}", m_CurCmdBufIdx);

            m_LastSubmissionSignal = m_AllSubmissionSignal[m_CurCmdBufIdx];
            m_InjectedSignal = VK_NULL_HANDLE;
            m_CurCmdBufIdx = INVALID_IDX;

            return true;
        }

        VkSemaphore CommandBufferCache::GetLastSubmissionSingal()
        {
            VkSemaphore res = VK_NULL_HANDLE;
            std::swap(res, m_LastSubmissionSignal);
            return res;
        }

        void CommandBufferCache::InjectDependency(VkSemaphore dependency)
        {
            m_InjectedSignal = dependency;
        }

        void CommandBufferCache::GC()
        {
            for (uint32_t i = 0; i < COMMAND_BUFFER_COUNT; ++ i)
            {
                if (m_AllCmdBuf[i].State.load(std::memory_order::seq_cst) != CommandBuffer::State::NonExist)
                {
                    // time out is 0, just query the status of fence (execution finished or not in other words)
                    VkResult res = vkWaitForFences(m_Device, 1, &m_AllCmdBuf[i].Fence, VK_TRUE, 0);
                    if (res == VK_SUCCESS)
                    {
                        COUST_CORE_TRACE("Release command buffer {}", i);
                        vkFreeCommandBuffers(m_Device, m_Pool, 1, &m_AllCmdBuf[i].Handle);
                        m_AllCmdBuf[i].Handle = VK_NULL_HANDLE;
                        vkDestroyFence(m_Device, m_AllCmdBuf[i].Fence, nullptr);
                        m_AllCmdBuf[i].Fence = VK_NULL_HANDLE;
                        m_AllCmdBuf[i].State.store(CommandBuffer::State::NonExist, std::memory_order::seq_cst);

                        ++ m_AvailableCmdBuf;
                    }
                }
            }
        }

        void CommandBufferCache::Wait()
        {
            VkFence fencesToWait[COMMAND_BUFFER_COUNT];
            uint32_t idx = 0;
            for (uint32_t i = 0; i < COMMAND_BUFFER_COUNT; ++ i)
            {
                // the current command buffer might not even be submitted, skip it
                if (i != m_CurCmdBufIdx && m_AllCmdBuf[i].State.load(std::memory_order::seq_cst) != CommandBuffer::State::NonExist)
                {
                    COUST_CORE_TRACE("Wait command buffer {}", i);
                    fencesToWait[idx++] = m_AllCmdBuf[i].Fence;
                    m_AllCmdBuf[i].State.store(CommandBuffer::State::Invalid, std::memory_order::relaxed);
                }
            }
            if (idx > 0)
                vkWaitForFences(m_Device, idx, fencesToWait, VK_TRUE, UINT64_MAX);
        }

        void CommandBufferCache::SetCommandBufferChangedCallback(CommandBufferChangedCallback&& callback)
        {
            m_CmdBufChangedCallback = callback;
        }
        
        bool CommandBufferCache::IsValid() const { return m_Pool != VK_NULL_HANDLE; }
}
