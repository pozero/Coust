#include "pch.h"

#include "Coust/Render/Vulkan/Refrigerator.h"
#include "Coust/Render/Vulkan/VulkanCommand.h"

namespace Coust::Render::VK
{
    static constexpr uint32_t TIME_BEFORE_RELEASE = CommandBufferCache::COMMAND_BUFFER_COUNT;

    void Refrigerator::Reset() noexcept
    {
        for (const auto& p : m_Resources)
        {
            p.second.deletor(p.first);
        }
        m_Resources.clear();
    }

    void Refrigerator::Acquire(void* resource) noexcept
    {
        auto iter = m_Resources.find(resource);
        if (iter == m_Resources.end())
            return;
        
        ++ iter->second.refCount; 
        iter->second.remainFrame = TIME_BEFORE_RELEASE;
    }

    void Refrigerator::Release(void* resource) noexcept
    {
        auto iter = m_Resources.find(resource);
        if (iter == m_Resources.end())
            return;
        
        -- iter->second.refCount; 
    }

    void Refrigerator::GC()
    {
        // Remain frame count down
        for (auto& p : m_Resources)
        {
            if (p.second.refCount > 0 && p.second.remainFrame > 0)
                if (-- p.second.remainFrame == 0)
                    Release(p.first);
        }

        // Pour the expired to trash
        decltype(m_Resources) resources;
        for (auto& p : m_Resources)
        {
            if (p.second.refCount == 0)
                p.second.deletor(p.first);
            else  
                resources.emplace(p.first, p.second);
        }
        m_Resources.swap(resources);
    }
}