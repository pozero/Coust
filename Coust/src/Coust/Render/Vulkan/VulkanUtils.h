#pragma once

#include "Coust/Core/Logger.h"

#include <volk.h>

#include <vector>

#define VK_CHECK(func, ...)																													    \
    do																																	        \
    {																																	        \
        VkResult err = func;																											        \
        COUST_CORE_PANIC_IF(err != VK_SUCCESS, "Vulkan Func {} return {}\n\t{}", #func, ToString(err), fmt::format(__VA_ARGS__));				\
    } while (false)																														

namespace Coust::Render::VK
{
    template <typename T>
    inline uint32_t ToU32(T value)
        requires std::is_arithmetic<T>::value
    {
        COUST_CORE_ASSERT(static_cast<uintmax_t>(value) <= static_cast<uintmax_t>(std::numeric_limits<uint32_t>::max()), 
                          "ToU32() failed, value is too big to be converted to uint32_t");
        return static_cast<uint32_t>(value);
    }

    VkFormat GetNomalizedFormat(VkFormat singleByteFormat) noexcept;

    // Converting the vulkan srgb format to its corresponding unsigned normalized byte format, as the spec says:
    // VK_FORMAT_R8G8B8_SRGB specifies a three-component, 24-bit unsigned normalized format ...
    VkFormat UnpackSRGBFormat(VkFormat srgbFormat) noexcept;

    VkImageMemoryBarrier2 ImageBlitTransition(VkImageMemoryBarrier2 transition) noexcept;

    void TransitionImageLayout(VkCommandBuffer cmdBuf, VkImageMemoryBarrier2 transition) noexcept;

    uint32_t GetBytePerPixelFromFormat(VkFormat format) noexcept;

    bool IsDepthOnlyFormat(VkFormat format) noexcept;

    bool IsDepthStencilFormat(VkFormat format) noexcept;

    const char* ToString(VkResult result) noexcept;
    
    const char* ToString(VkObjectType objType) noexcept;
    
    const char* ToString(VkAccessFlagBits bit) noexcept;
    
    const char* ToString(VkShaderStageFlagBits bit) noexcept;
    
    const char* ToString(VkCommandBufferLevel level) noexcept;
    
    const char* ToString(VkDescriptorType type) noexcept;
    
    const char* ToString(VkBufferUsageFlagBits bit) noexcept;

    const char* ToString(VkImageUsageFlagBits bit) noexcept;

    const char* ToString(VkFormat format) noexcept;
    
    template <typename Flag, typename FlagBit>
    inline std::string ToString(Flag flags) noexcept
        requires std::is_arithmetic<Flag>::value && std::is_enum<FlagBit>::value
    {
        std::string res{};
        for (FlagBit bit = (FlagBit) 1u; uintmax_t(bit) > 0u; bit = FlagBit(uintmax_t(bit) << 1))
        {
            if (bit & flags)
            {
                if (!res.empty())
                    res.append(" | ");
                res.append(ToString(bit));
            }
        }
        return res;
    }
}

namespace std
{
    template <>
    struct hash<VkDescriptorBufferInfo>
    {
        std::size_t operator()(const VkDescriptorBufferInfo& key) const noexcept;
    };

    template <>
    struct hash<VkDescriptorImageInfo>
    {
        std::size_t operator()(const VkDescriptorImageInfo& key) const noexcept;
    };

    template <>
    struct hash<VkWriteDescriptorSet>
    {
        std::size_t operator()(const VkWriteDescriptorSet& key) const noexcept;
    };
}