#pragma once

#include "utils/Compiler.h"
#include "render/vulkan/utils/VulkanAttachment.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {

class VulkanRenderPass {
public:
    VulkanRenderPass() = delete;
    VulkanRenderPass(VulkanRenderPass const&) = delete;
    VulkanRenderPass& operator=(VulkanRenderPass const&) = delete;

public:
    static uint32_t constexpr object_type = VK_OBJECT_TYPE_RENDER_PASS;

    VkDevice get_device() const noexcept;

    VkRenderPass get_handle() const noexcept;

    // Each render pass can have at most 2 subpasses (e.g. deferred rendering)
    struct Param {
        // `VK_FORMAT_UNDEFINED` means don't use this attachment
        std::array<VkFormat, MAX_ATTACHMENT_COUNT> color_formats{
            VK_FORMAT_UNDEFINED,
            VK_FORMAT_UNDEFINED,
            VK_FORMAT_UNDEFINED,
            VK_FORMAT_UNDEFINED,
            VK_FORMAT_UNDEFINED,
            VK_FORMAT_UNDEFINED,
            VK_FORMAT_UNDEFINED,
            VK_FORMAT_UNDEFINED,
        };
        VkFormat depth_format = VK_FORMAT_UNDEFINED;
        AttachmentFlags clear_mask = 0u;
        AttachmentFlags discard_start_mask = 0u;
        AttachmentFlags discard_end_mask = 0u;
        VkSampleCountFlagBits sample = VK_SAMPLE_COUNT_1_BIT;
        uint8_t resolve_mask = 0u;
        uint8_t input_attachment_mask = 0u;
        bool depth_resolve = false;
    };

public:
    VulkanRenderPass(VkDevice dev, Param const& param) noexcept;

    VulkanRenderPass(VulkanRenderPass&&) noexcept = default;

    VulkanRenderPass& operator=(VulkanRenderPass&&) noexcept = default;

    ~VulkanRenderPass() noexcept;

    VkExtent2D get_render_area_granularity() const noexcept;

private:
    VkDevice m_dev = VK_NULL_HANDLE;

    VkRenderPass m_handle = VK_NULL_HANDLE;
};

}  // namespace render
}  // namespace coust

namespace std {

template <>
struct hash<coust::render::VulkanRenderPass::Param> {
    std::size_t operator()(
        coust::render::VulkanRenderPass::Param const& key) const noexcept;
};

template <>
struct equal_to<coust::render::VulkanRenderPass::Param> {
    bool operator()(coust::render::VulkanRenderPass::Param const& left,
        coust::render::VulkanRenderPass::Param const& right) const noexcept;
};

}  // namespace std
