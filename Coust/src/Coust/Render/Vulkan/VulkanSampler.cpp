#include "pch.h"

#include "Coust/Render/Vulkan/VulkanSampler.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"

namespace Coust::Render::VK 
{
    SamplerCache::SamplerCache(const Context& ctx)
        : m_Ctx(ctx), m_HitCounter("Sampler")
    {
    }

    void SamplerCache::Reset()
    {
        for (const auto& pair : m_CachedSamplers)
        {
            vkDestroySampler(m_Ctx.Device, pair.second, nullptr);
        }
        m_CachedSamplers.clear();
    }

    inline VkFilter GetMagFilter(SamplerCache::MagFilter mag)
    {
        switch (mag)
        {
            case SamplerCache::MagFilter::Nearest: return VK_FILTER_NEAREST;
            case SamplerCache::MagFilter::Linear: return VK_FILTER_LINEAR;
            default: return VK_FILTER_MAX_ENUM;
        }
    }

    inline VkFilter GetMinFilter(SamplerCache::MinFilter min)
    {
        switch (min)
        {
            case SamplerCache::MinFilter::Nearest:
            case SamplerCache::MinFilter::Nearest_MipMap_Nearest:
            case SamplerCache::MinFilter::Nearest_MipMap_Linear:
                return VK_FILTER_NEAREST;
            case SamplerCache::MinFilter::Linear:
            case SamplerCache::MinFilter::Linear_MipMap_Nearest:
            case SamplerCache::MinFilter::Linear_MipMap_Linear:
                return VK_FILTER_LINEAR;
            default:
                return VK_FILTER_MAX_ENUM;
        }
    }

    inline VkSamplerMipmapMode GetMipMapMode(SamplerCache::MinFilter min)
    {
        switch (min)
        {
            case SamplerCache::MinFilter::Nearest:
            case SamplerCache::MinFilter::Linear:
            case SamplerCache::MinFilter::Nearest_MipMap_Nearest:
            case SamplerCache::MinFilter::Linear_MipMap_Nearest:
                return VK_SAMPLER_MIPMAP_MODE_NEAREST;
            case SamplerCache::MinFilter::Nearest_MipMap_Linear:
            case SamplerCache::MinFilter::Linear_MipMap_Linear:
                return VK_SAMPLER_MIPMAP_MODE_LINEAR;
            default:
                return VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
        }
    }

    float GetMaxLod(SamplerCache::MinFilter min)
    {
        switch (min)
        {
            case SamplerCache::MinFilter::Nearest:
            case SamplerCache::MinFilter::Linear:
                // lod disabled, the spec says:
                // There are no Vulkan filter modes that directly correspond to OpenGL minification filters of GL_LINEAR or GL_NEAREST, 
                // but they can be emulated using VK_SAMPLER_MIPMAP_MODE_NEAREST, minLod = 0, and maxLod = 0.25, and using minFilter = VK_FILTER_LINEAR or minFilter = VK_FILTER_NEAREST, respectively.
                return 0.25f;
            case SamplerCache::MinFilter::Nearest_MipMap_Nearest:
            case SamplerCache::MinFilter::Nearest_MipMap_Linear:
            case SamplerCache::MinFilter::Linear_MipMap_Nearest:
            case SamplerCache::MinFilter::Linear_MipMap_Linear:
                // 2 ^ 12 = 8192, and 4K (the highest resolution of texture) is 4096 × 2160.
                return 12.0f;
            default:
                return 0.25f;
        }
    }

    VkSampler SamplerCache::Get(const SamplerInfo& info)
    {
        if (auto iter = m_CachedSamplers.find(info); iter != m_CachedSamplers.end())
        {
            m_HitCounter.Hit();

            return iter->second;
        }

        m_HitCounter.Miss();

        VkSamplerCreateInfo CI
        {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = GetMagFilter(info.mag),
            .minFilter = GetMinFilter(info.min),
            .mipmapMode = GetMipMapMode(info.min),
            .addressModeU = info.modeU,
            .addressModeV = info.modeV,
            .addressModeW = info.modeW,
            .mipLodBias = 0.0f,
            .anisotropyEnable = info.maxAnisotropy > 0.0f ? VK_TRUE : VK_FALSE,
            .maxAnisotropy = info.maxAnisotropy,
            .compareEnable = info.compareMode == VK_COMPARE_OP_MAX_ENUM ? VK_FALSE : VK_TRUE,
            .compareOp = info.compareMode,
            .minLod = 0.0f,
            .maxLod = GetMaxLod(info.min),
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        };
        VkSampler s = VK_NULL_HANDLE;
        bool success = false;
        VK_REPORT(vkCreateSampler(m_Ctx.Device, &CI, nullptr, &s), success);
        if (!success)
            return VK_NULL_HANDLE;
        m_CachedSamplers.emplace(info, s);
        return s;
    }

}