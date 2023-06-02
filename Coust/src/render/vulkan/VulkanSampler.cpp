#include "pch.h"

#include "utils/math/Hash.h"
#include "render/vulkan/VulkanSampler.h"
#include "render/vulkan/utils/VulkanCheck.h"
#include "render/vulkan/utils/VulkanAllocation.h"

namespace coust {
namespace render {

constexpr VkFilter get_mag_filter(MagFilter mag) noexcept {
    switch (mag) {
        case MagFilter::nearest:
            return VK_FILTER_NEAREST;
        case MagFilter::linear:
            return VK_FILTER_LINEAR;
    }
    ASSUME(0);
}

constexpr VkFilter get_min_filter(MinFilter min) noexcept {
    switch (min) {
        case MinFilter::nearest_nomipmap:
        case MinFilter::nearest_mipmap_nearest:
        case MinFilter::nearest_mipmap_linear:
            return VK_FILTER_NEAREST;
        case MinFilter::linear_nomipmap:
        case MinFilter::linear_mipmap_nearest:
        case MinFilter::linear_mipmap_linear:
            return VK_FILTER_LINEAR;
    }
    ASSUME(0);
}

constexpr VkSamplerMipmapMode get_mipmap_mode(MinFilter min) noexcept {
    switch (min) {
        case MinFilter::nearest_nomipmap:
        case MinFilter::linear_nomipmap:
        case MinFilter::nearest_mipmap_nearest:
        case MinFilter::linear_mipmap_nearest:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case MinFilter::nearest_mipmap_linear:
        case MinFilter::linear_mipmap_linear:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
    ASSUME(0);
}

constexpr float get_max_lod(MinFilter min) noexcept {
    switch (min) {
        case MinFilter::nearest_nomipmap:
        case MinFilter::linear_nomipmap:
            // lod disabled, the spec says:
            // There are no Vulkan filter modes that directly correspond to
            // OpenGL minification filters of GL_LINEAR or GL_NEAREST, but they
            // can be emulated using VK_SAMPLER_MIPMAP_MODE_NEAREST, minLod = 0,
            // and maxLod = 0.25, and using minFilter = VK_FILTER_LINEAR or
            // minFilter = VK_FILTER_NEAREST, respectively.
            return 0.25f;
        case MinFilter::nearest_mipmap_nearest:
        case MinFilter::nearest_mipmap_linear:
        case MinFilter::linear_mipmap_nearest:
        case MinFilter::linear_mipmap_linear:
            // 2 ^ 12 = 8192, and 4K (the highest resolution of texture) is 4096
            // × 2160.
            return 12.0f;
    }
    ASSUME(0);
}

VulkanSamplerCache::VulkanSamplerCache(VkDevice dev) noexcept
    : m_dev(dev), m_hit_counter("Vulkan Sampler Cache [Sampler]") {
}

VkSampler VulkanSamplerCache::get(VulkanSamplerParam const &param) noexcept {
    auto iter = m_samplers.find(param);
    if (iter != m_samplers.end()) {
        m_hit_counter.hit();
        return iter->second;
    } else {
        m_hit_counter.miss();
        VkSamplerCreateInfo sampler_info{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = get_mag_filter(param.mag),
            .minFilter = get_min_filter(param.min),
            .mipmapMode = get_mipmap_mode(param.min),
            .addressModeU = param.mode_u,
            .addressModeV = param.mode_v,
            .addressModeW = param.mode_w,
            .mipLodBias = 0.0f,
            .anisotropyEnable =
                param.max_anisotropy > 0.0f ? VK_TRUE : VK_FALSE,
            .maxAnisotropy = param.max_anisotropy,
            .compareEnable =
                param.compare == VK_COMPARE_OP_MAX_ENUM ? VK_FALSE : VK_TRUE,
            .compareOp = param.compare,
            .minLod = 0.0f,
            .maxLod = get_max_lod(param.min),
            .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        };
        VkSampler sampler = VK_NULL_HANDLE;
        COUST_VK_CHECK(vkCreateSampler(m_dev, &sampler_info,
                           COUST_VULKAN_ALLOC_CALLBACK, &sampler),
            "");
        m_samplers.emplace(param, sampler);
        return sampler;
    }
}

void VulkanSamplerCache::reset() {
    for (auto const &[p, sampler] : m_samplers) {
        vkDestroySampler(m_dev, sampler, COUST_VULKAN_ALLOC_CALLBACK);
    }
    m_samplers.clear();
}

}  // namespace render
}  // namespace coust

namespace std {

std::size_t hash<coust::render::VulkanSamplerParam>::operator()(
    coust::render::VulkanSamplerParam const &key) const noexcept {
    size_t h = coust::calc_std_hash(key.mag);
    coust::hash_combine(h, key.min);
    coust::hash_combine(h, key.mode_u);
    coust::hash_combine(h, key.mode_v);
    coust::hash_combine(h, key.mode_w);
    coust::hash_combine(h, key.compare);
    coust::hash_combine(h, key.max_anisotropy);
    return h;
}

WARNING_PUSH
CLANG_DISABLE_WARNING("-Wfloat-equal")
bool equal_to<coust::render::VulkanSamplerParam>::operator()(
    coust::render::VulkanSamplerParam const &left,
    coust::render::VulkanSamplerParam const &right) const noexcept {
    return left.mag == right.mag && left.min == right.min &&
           left.mode_u == right.mode_u && left.mode_v == right.mode_v &&
           left.mode_w == right.mode_w && left.compare == right.compare &&
           left.max_anisotropy == right.max_anisotropy;
}
WARNING_POP

}  // namespace std
