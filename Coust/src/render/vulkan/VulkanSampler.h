#pragma once

#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "utils/Compiler.h"
#include "render/vulkan/utils/CacheSetting.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {

enum class MagFilter {
    nearest,
    linear,
};
// this filter is used in min and mipmap
enum class MinFilter {
    nearest_nomipmap,
    linear_nomipmap,
    nearest_mipmap_nearest,
    linear_mipmap_nearest,
    nearest_mipmap_linear,
    linear_mipmap_linear,
};

struct VulkanSamplerParam {
    MagFilter mag = MagFilter::nearest;
    MinFilter min = MinFilter::nearest_nomipmap;
    VkSamplerAddressMode mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VkSamplerAddressMode mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VkSamplerAddressMode mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VkCompareOp compare = VK_COMPARE_OP_MAX_ENUM;
    float max_anisotropy = 16.0f;
};

}  // namespace render
}  // namespace coust

namespace std {

template <>
struct hash<coust::render::VulkanSamplerParam> {
    std::size_t operator()(
        coust::render::VulkanSamplerParam const &key) const noexcept;
};

template <>
struct equal_to<coust::render::VulkanSamplerParam> {
    bool operator()(coust::render::VulkanSamplerParam const &left,
        coust::render::VulkanSamplerParam const &right) const noexcept;
};

}  // namespace std

namespace coust {
namespace render {

class VulkanSamplerCache {
public:
    VulkanSamplerCache() = delete;
    VulkanSamplerCache(VulkanSamplerCache &&) = delete;
    VulkanSamplerCache(VulkanSamplerCache const &) = delete;
    VulkanSamplerCache &operator=(VulkanSamplerCache &&) = delete;
    VulkanSamplerCache &operator=(VulkanSamplerCache const &) = delete;

public:
public:
    VulkanSamplerCache(VkDevice dev) noexcept;

    VkSampler get(VulkanSamplerParam const &param) noexcept;

    void reset();

private:
    VkDevice m_dev = VK_NULL_HANDLE;

    memory::robin_map<VulkanSamplerParam, VkSampler, DefaultAlloc> m_samplers{
        get_default_alloc()};

    CacheHitCounter m_hit_counter;
};

}  // namespace render
}  // namespace coust
