#include "pch.h"

#include "Coust/Render/Vulkan/StagePool.h"
#include "Coust/Render/Vulkan/VulkanCommand.h"

namespace Coust::Render::VK 
{
    static constexpr uint32_t TIME_BEFORE_RELEASE = CommandBufferCache::COMMAND_BUFFER_COUNT;

    StagePool::StagePool(const Context& ctx) noexcept
        : m_Ctx(ctx), m_Timer(TIME_BEFORE_RELEASE), m_BufferHitCounter("StagePool Buffer"), m_ImageHitCounter("StagePool Image")
    {
    }

    std::shared_ptr<Buffer> StagePool::AcquireStagingBuffer(VkDeviceSize numBytes) noexcept
    {
        // Find the buffer meeting the size requirement with the smallest capcacity
        if (auto iter = m_FreeStagingBuf.lower_bound(numBytes); iter != m_FreeStagingBuf.end())
        {
            m_BufferHitCounter.Hit();
            auto stage = iter->second;  
            m_FreeStagingBuf.erase(iter);
            stage.lastAccessed = m_Timer.CurrentCount(),
            m_UsedStagingBuf.push_back(stage);
            return stage.buf;
        }

        m_ImageHitCounter.Miss();

        StagingBuffer buf{};
        Buffer::ConstructParam param
        {
            .ctx = m_Ctx,
            .size = numBytes,
            .bufferFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .usage = Buffer::Usage::Staging,
            .scopeName = "VK::StagePool",
        };
        COUST_CORE_PANIC_IF(!Buffer::Create(buf.buf, param), "Can't create staging buffer");
        buf.lastAccessed = m_Timer.CurrentCount();
        m_UsedStagingBuf.push_back(buf);
        return buf.buf;
    }

    std::shared_ptr<HostImage> StagePool::AcquireStagingImage(VkFormat format, uint32_t width, uint32_t height) noexcept
    {
        for (auto iter = m_FreeStagingImage.begin(); iter != m_FreeStagingImage.end(); ++ iter)
        {
            const auto& i = *iter->image;
            if (i.GetFormat() == format && i.GetWidth() == width && i.GetHeight() == height)
            {
                m_ImageHitCounter.Hit();
                auto stage = *iter;
                m_FreeStagingImage.erase(iter);
                stage.lastAccessed = m_Timer.CurrentCount(),
                m_UsedStagingImage.push_back(stage);
                return stage.image;
            }
        }

        m_ImageHitCounter.Miss();

        StagingImage image{};
        HostImage::ConstructParam param
        {
            .ctx = m_Ctx,
            .format = format,
            .width = width,
            .height = height,
        };
        COUST_CORE_PANIC_IF(!HostImage::Create(image.image, param), "Can't create staging image");
        image.lastAccessed = m_Timer.CurrentCount();
        m_UsedStagingImage.push_back(image);
        return image.image;
    }

    void StagePool::GC() noexcept
    {
        m_Timer.Tick();

        // release unused staging buffer in the free list
        {
            std::multimap<VkDeviceSize, StagingBuffer> freeStageBuf{};
            freeStageBuf.swap(m_FreeStagingBuf);
            for (auto& pair : freeStageBuf)
            {
                if (!m_Timer.ShouldEvict(pair.second.lastAccessed))
                    m_FreeStagingBuf.insert(pair);
            }
        }

        // move the unused staging buffer to the free list
        {
            std::vector<StagingBuffer> usedStageBuf;
            usedStageBuf.swap(m_UsedStagingBuf);
            for (auto& buf : usedStageBuf)
            {
                // the buffer should be moved to the free list
                if (m_Timer.ShouldEvict(buf.lastAccessed))  
                {
                    // update its last access time to prevent it from being deleted immdiately at next gc
                    buf.lastAccessed = m_Timer.CurrentCount();
                    m_FreeStagingBuf.insert(std::make_pair(buf.buf->GetSize(), buf));
                }
                else  
                    m_UsedStagingBuf.push_back(buf);
            }
        }
        
        // release unused staging image in the free list
        {
            std::vector<StagingImage> freeStageImage; 
            freeStageImage.swap(m_FreeStagingImage);
            for (auto& image : freeStageImage)
            {
                if (!m_Timer.ShouldEvict(image.lastAccessed))
                    m_FreeStagingImage.push_back(image);
            }
        }
        
        // move the unused staging image to the free list
        {
            std::vector<StagingImage> usedStageImage;
            usedStageImage.swap(m_UsedStagingImage);
            for (auto& image : usedStageImage)
            {
                if (m_Timer.ShouldEvict(image.lastAccessed))
                {
                    image.lastAccessed = m_Timer.CurrentCount();
                    m_FreeStagingImage.push_back(image);
                }
                else  
                    m_UsedStagingImage.push_back(image);
            }
        }
    }
    
    void StagePool::Reset() noexcept
    {
        m_FreeStagingBuf.clear();
        m_UsedStagingBuf.clear();
        m_FreeStagingImage.clear();
        m_UsedStagingImage.clear();
    }
}