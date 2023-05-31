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
    VulkanRenderPass(VulkanRenderPass&&) = delete;
    VulkanRenderPass(VulkanRenderPass const&) = delete;
    VulkanRenderPass& operator=(VulkanRenderPass&&) = delete;
    VulkanRenderPass& operator=(VulkanRenderPass const&) = delete;

public:
    static uint32_t constexpr object_type = VK_OBJECT_TYPE_RENDER_PASS;

    VkDevice get_device() const noexcept;

    VkRenderPass get_handle() const noexcept;

private:
    VkDevice m_dev = VK_NULL_HANDLE;

    VkRenderPass m_handle = VK_NULL_HANDLE;
};

}  // namespace render
}  // namespace coust
