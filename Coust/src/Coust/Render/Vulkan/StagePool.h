#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"
#include "Coust/Render/Vulkan/VulkanMemory.h"

#include <list>
#include <map>

namespace Coust::Render::VK 
{
    /**
     * @brief Manages a pool of two types of stages: buffer and image
     *        Will periodically release stages that haven't been used for a while
     */
    class StagePool
    {
    public:
        
    public:
        explicit StagePool(const Context& ctx);

        std::shared_ptr<Buffer> AcquireStagingBuffer(VkDeviceSize numBytes);

        std::shared_ptr<HostImage> AcquireStagingImage(VkFormat format, uint32_t width, uint32_t height);

        // Release pointer to the unused (maybe) staging buffers & staging images.
        void GC();
        
        // Reset all the internal states of this pool. It's dangerous to call it when there're still staging objects in use.
        void Reset();

    private:
        // Ideally, the life cycle of staging buffer & staging image should be managed by stage pool completely.
        // But for the sake of memory safety, we still use shared pointer here.
        struct StagingBuffer
        {
            std::shared_ptr<Buffer> buf;
            uint32_t lastAccessed;
        };

        struct StagingImage
        {
            std::shared_ptr<HostImage> image;
            uint32_t lastAccessed;
        };

    private:
        const Context& m_Ctx;

        // buffer size => staging buffer
        std::multimap<VkDeviceSize, StagingBuffer> m_FreeStagingBuf;
        std::list<StagingBuffer> m_UsedStagingBuf;

        std::list<StagingImage> m_FreeStagingImage;
        std::list<StagingImage> m_UsedStagingImage;

        // storing current frame count, used for recycling memory
        EvictTimer m_Timer;
    };
}