#pragma once

#include "utils/Compiler.h"
#include "render/vulkan/utils/VulkanAttachment.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {

class VulkanImageView;
class VulkanRenderPass;

class VulkanFramebuffer {
public:
    VulkanFramebuffer() = delete;
    VulkanFramebuffer(VulkanFramebuffer const &) = delete;
    VulkanFramebuffer &operator=(VulkanFramebuffer const &) = delete;

public:
    static uint32_t constexpr object_type = VK_OBJECT_TYPE_RENDER_PASS;

    VkDevice get_device() const noexcept;

    VkFramebuffer get_handle() const noexcept;

    struct Param {
        uint32_t width;
        uint32_t height;
        uint32_t layers;
        std::array<const VulkanImageView *, MAX_ATTACHMENT_COUNT> colors{
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
        };
        std::array<const VulkanImageView *, MAX_ATTACHMENT_COUNT> resolves{
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
        };
        const VulkanImageView *depth = nullptr;
        const VulkanImageView *depth_resolve = nullptr;
        const VulkanRenderPass *render_pass = nullptr;
    };

public:
    VulkanFramebuffer(VkDevice dev, Param const &param) noexcept;

    VulkanFramebuffer(VulkanFramebuffer &&other) noexcept;

    VulkanFramebuffer &operator=(VulkanFramebuffer &&other) noexcept;

    ~VulkanFramebuffer() noexcept;

    VulkanRenderPass const &get_render_pass() const noexcept;

private:
    VkDevice m_dev = VK_NULL_HANDLE;

    VkFramebuffer m_handle = VK_NULL_HANDLE;

    const VulkanRenderPass *m_render_pass = nullptr;
};

}  // namespace render
}  // namespace coust

namespace std {

template <>
struct hash<coust::render::VulkanFramebuffer::Param> {
    std::size_t operator()(
        coust::render::VulkanFramebuffer::Param const &key) const noexcept;
};

template <>
struct equal_to<coust::render::VulkanFramebuffer::Param> {
    bool operator()(coust::render::VulkanFramebuffer::Param const &left,
        coust::render::VulkanFramebuffer::Param const &right) const noexcept;
};

}  // namespace std
