#pragma once

#include "Coust/Core/Logger.h"

#include <volk.h>

#include <vector>

#define VK_CHECK(func)																													\
    do																																	\
    {																																	\
        VkResult err = func;																											\
        if (err != VK_SUCCESS)																											\
        {																																\
            COUST_CORE_ERROR("File {0}, Line{1}, Vulkan Func {2} return {3}", __FILE__, __LINE__, #func, ToString(err));				\
            return false;																												\
        }																																\
    } while (false)																														

#define VK_REPORT(func, status)																											\
    do																																	\
    {																																	\
        VkResult err = func;																											\
        if (err != VK_SUCCESS)																											\
        {																																\
            status = false;																												\
            COUST_CORE_ERROR("File {0}, Line{1}, Vulkan Func {2} return {3}", __FILE__, __LINE__, #func, ToString(err));				\
        }																																\
        else																															\
            status = true;																												\
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

    VkFormat GetNomalizedFormat(VkFormat singleByteFormat);

    // Converting the vulkan srgb format to its corresponding unsigned normalized byte format, as the spec says:
    // VK_FORMAT_R8G8B8_SRGB specifies a three-component, 24-bit unsigned normalized format ...
    VkFormat UnpackSRGBFormat(VkFormat srgbFormat);

    VkImageMemoryBarrier2 ImageBlitTransition(VkImageMemoryBarrier2 transition);

    void TransitionImageLayout(VkCommandBuffer cmdBuf, VkImageMemoryBarrier2 transition);

    uint32_t GetBytePerPixelFromFormat(VkFormat format);

    bool IsDepthOnlyFormat(VkFormat format);

    bool IsDepthStencilFormat(VkFormat format);

    const char* ToString(VkResult result);
    
    const char* ToString(VkObjectType objType);
    
    const char* ToString(VkAccessFlagBits bit);
    
    const char* ToString(VkShaderStageFlagBits bit);
    
    const char* ToString(VkCommandBufferLevel level);
    
    const char* ToString(VkDescriptorType type);
    
    const char* ToString(VkBufferUsageFlagBits bit);

    const char* ToString(VkImageUsageFlagBits bit);

    const char* ToString(VkFormat format);
    
    template <typename Flag, typename FlagBit>
    inline std::string ToString(Flag flags)
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
        std::size_t operator()(const VkDescriptorBufferInfo& key) const;
    };

    template <>
    struct hash<VkDescriptorImageInfo>
    {
        std::size_t operator()(const VkDescriptorImageInfo& key) const;
    };

    template <>
    struct hash<VkWriteDescriptorSet>
    {
        std::size_t operator()(const VkWriteDescriptorSet& key) const;
    };
}