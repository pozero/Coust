#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"

#include "Coust/Utils/Hash.h"

#include <unordered_map>

namespace Coust::Render::VK 
{
    class SamplerCache
    {
    public:
        SamplerCache() = delete;
        SamplerCache(SamplerCache&&) = delete;
        SamplerCache(const SamplerCache&) = delete;
        SamplerCache& operator=(SamplerCache&&) = delete;
        SamplerCache& operator=(const SamplerCache&) = delete;

    public:
        enum class MagFilter 
        {
            Nearest,
            Linear,
        };
        // this filter is used in min and mipmap
        enum class MinFilter
        {
            Nearest,
            Linear,
            Nearest_MipMap_Nearest,
            Linear_MipMap_Nearest,
            Nearest_MipMap_Linear,
            Linear_MipMap_Linear,
        };
    
    public:
        SamplerCache(const Context& ctx) noexcept;

        void Reset() noexcept;

        struct SamplerInfo 
        {
            MagFilter                mag = MagFilter::Nearest;
            MinFilter                min = MinFilter::Nearest;
            VkSamplerAddressMode     modeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            VkSamplerAddressMode     modeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            VkSamplerAddressMode     modeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            VkCompareOp              compareMode = VK_COMPARE_OP_MAX_ENUM;
            float                    maxAnisotropy = 16.0f;
        };

        VkSampler Get(const SamplerInfo& info) noexcept;

    private:
        const Context& m_Ctx;

        std::unordered_map<SamplerInfo, VkSampler, Hash::HashFn<SamplerInfo>, Hash::EqualFn<SamplerInfo>> m_CachedSamplers;

        CacheHitCounter m_HitCounter;
    };
}